#ifndef NAVGRAPH_HPP
#define NAVGRAPH_HPP

#include <memory>
#include <vector>
#include <unordered_map>
#include <random>
#include <cmath>
#include <sstream>
#include <stdexcept>

#include <jsoncpp/json/json.h>
#include <opencv2/opencv.hpp>

#ifdef OSMESA_RENDERING
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/osmesa.h>
#elif defined (EGL_RENDERING)
#include <epoxy/gl.h>
#include <EGL/egl.h>
#else
#include <GL/glew.h>
#endif

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace mattersim {

    static void assertOpenGLError(const std::string& msg) {
      GLenum error = glGetError();
      if (error != GL_NO_ERROR) {
        std::stringstream s;
        s << "OpenGL error 0x" << std::hex << error << " at " << msg;
        throw std::runtime_error(s.str());
      }
    }
#ifdef EGL_RENDERING
    static void assertEGLError(const std::string& msg) {
      EGLint error = eglGetError();

      if (error != EGL_SUCCESS) {
        std::stringstream s;
        s << "EGL error 0x" << std::hex << error << " at " << msg;
        throw std::runtime_error(s.str());
      }
    }
#endif

    /**
     * Navigation graph indicating which panoramic viewpoints are adjacent, and also 
     * containing (optionally pre-loaded) skybox / cubemap images and textures.
     * Class is a singleton to ensure images and textures are only loaded once.
     */
    class NavGraph final {

    private:

        NavGraph(const std::string& navGraphPath, const std::string& datasetPath, 
                bool preloadImages, bool renderDepth, int randomSeed, unsigned int cacheSize);

        ~NavGraph();

    public:
        // Delete the default, copy and move constructors
        NavGraph() = delete;
        NavGraph(const NavGraph&) = delete;
        NavGraph& operator=(const NavGraph&) = delete;
        NavGraph(NavGraph&&) = delete;
        NavGraph& operator=(NavGraph&&) = delete;

        /**
         * First call will load the navigation graph from disk and (optionally) preload the 
         * cubemap images into memory.
         * @param navGraphPath - directory containing json viewpoint connectivity graphs
         * @param datasetPath - directory containing a data directory for each Matterport scan id
         * @param preloadImages - if true, all cubemap images will be loaded into CPU memory immediately
         * @param renderDepth - if true, depth map images are also required
         * @param randomSeed - only used for randomViewpoint function
         * @param cacheSize - number of pano textures to keep in GPU memory
         */
        static NavGraph& getInstance(const std::string& navGraphPath, const std::string& datasetPath, 
                bool preloadImages, bool renderDepth, int randomSeed, unsigned int cacheSize);
  
        /**
         * Select a random viewpoint from a scan
         */
        const std::string& randomViewpoint(const std::string& scanId);
                      
        /**
         * Find the index of a selected viewpointId
         */
        unsigned int index(const std::string& scanId, const std::string& viewpointId) const;

        /**
         * ViewpointId of a selected viewpoint index
         */
        const std::string& viewpoint(const std::string& scanId, unsigned int ix) const;

        /**
         * Camera rotation matrix for a selected viewpoint index
         */
        const glm::mat4& cameraRotation(const std::string& scanId, unsigned int ix) const;

        /**
         * Camera position vector for a selected viewpoint index
         */
        const glm::vec3& cameraPosition(const std::string& scanId, unsigned int ix) const;

        /**
         * Return a list of other viewpoint indices that are reachable from a selected viewpoint index
         */
        std::vector<unsigned int> adjacentViewpointIndices(const std::string& scanId, unsigned int ix) const;

        /**
         * Get cubemap RGB (and optionally, depth) textures for a selected viewpoint index
         */
        std::pair<GLuint, GLuint> cubemapTextures(const std::string& scanId, unsigned int ix);

        /**
         * Free GPU memory associated with this viewpoint's textures
         */
        void deleteCubemapTextures(const std::string& scanId, unsigned int ix);


    protected:

        /**
         * Helper class representing nodes in the navigation graph and their cubemap textures.
         */
        class Location {

        public:
            /**
             * Construct a location object from a json struct
             * @param viewpoint - json struct
             * @param skyboxDir - directory containing a data directory for each Matterport scan id
             * @param preload - if true, all cubemap images will be loaded into CPU memory immediately
             * @param depth - if true, depth textures will also be provided
             */
            Location(const Json::Value& viewpoint, const std::string& skyboxDir, bool preload, bool depth);

            Location() = delete; // no default constructor

            /**
             * Return the cubemap RGB (and optionally, depth) textures for this viewpoint, which will 
             * be loaded from CPU memory or disk if necessary
             */
            std::pair<GLuint, GLuint> cubemapTextures();

            /**
             * Free GPU memory associated with RGB and depth textures at this location
             */
            void deleteCubemapTextures();

            std::string viewpointId;        //! Unique Matterport identifier for every pano
            bool included;                  //! Some duplicated viewpoints have been excluded
            glm::mat4 rot;                  //! Camera pose rotation component
            glm::vec3 pos;                  //! Camera pose translation component
            std::vector<bool> unobstructed; //! Connections to other graph locations

        protected:

            /**
             * Load RGB (and optionally, depth) cubemap images from disk into CPU memory
             */
            void loadCubemapImages();

            /**
             * Create RGB (and optionally, depth) textures from cubemap images (e.g., in GPU memory)
             */
            void loadCubemapTextures();

            GLuint cubemap_texture;
            GLuint depth_texture;
            cv::Mat xpos;                   //! RGB images for faces of the cubemap
            cv::Mat xneg;
            cv::Mat ypos;
            cv::Mat yneg;
            cv::Mat zpos;
            cv::Mat zneg;
            cv::Mat xposD;                   //! Depth images for faces of the cubemap
            cv::Mat xnegD;
            cv::Mat yposD;
            cv::Mat ynegD;
            cv::Mat zposD;
            cv::Mat znegD;
            bool im_loaded;
            bool includeDepth;
            std::string skyboxDir;          //! Path to skybox images
        };
        typedef std::shared_ptr<Location> LocationPtr;


        /**
         * Helper class implementing a LRU cache for cubemap textures.
         */
        class TextureCache {

        public:
            TextureCache(unsigned int size) : size(size) {
                cacheMap.reserve(size+1);
            }

            TextureCache() = delete; // no default constructor

            void add(LocationPtr loc) {
                auto map_it = cacheMap.find(loc);
                if (map_it != cacheMap.end()) {
                    // Remove entry from middle of list
                    cacheList.erase(map_it->second);
                    cacheMap.erase(map_it);
                }
                // Add element to list and save iterator on map
                auto list_it = cacheList.insert(cacheList.begin(), loc);
                cacheMap.emplace(loc, list_it);
                if (cacheMap.size() >= size) {
                    removeEldest();
                }
            }

            void removeEldest() {
                if (cacheMap.empty()) {
                    throw std::runtime_error("MatterSim: TextureCache is empty");
                }
                LocationPtr loc = cacheList.back();
                loc->deleteCubemapTextures();
                cacheMap.erase(loc);
                cacheList.pop_back();
            }

        private:
            unsigned int size;
            std::unordered_map<LocationPtr, std::list<LocationPtr>::iterator > cacheMap;
            std::list<LocationPtr> cacheList;
        };

        
        std::map<std::string, std::vector<LocationPtr> > scanLocations;
        std::default_random_engine generator;
        TextureCache cache;
    };

}

#endif
