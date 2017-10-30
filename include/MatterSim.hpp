#ifndef MATTERSIM_HPP
#define MATTERSIM_HPP

#include <memory>
#include <vector>
#include <random>
#include <time.h>

#include <opencv2/opencv.hpp>

#ifdef OSMESA_RENDERING
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/osmesa.h>
#else
#include <GL/glew.h>
#endif

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Benchmark.hpp"

namespace mattersim {
    struct Viewpoint {
        //! Viewpoint identifier
        std::string viewpointId;
        //! Viewpoint index
        unsigned int ix;
        //! 3D position
        cv::Point3f point;
    };

    typedef std::shared_ptr<Viewpoint> ViewpointPtr;

    /**
     * Simulator state class.
     */
    struct SimState {
        //! Building / scan environment identifier
        std::string scanId;
        //! Number of frames since the last newEpisode() call
        unsigned int step = 0;
        //! RGB image taken from the agent's current viewpoint
        cv::Mat rgb;
        //! Depth image taken from the agent's current viewpoint (not implemented)
        cv::Mat depth;
        //! Agent's current 3D location
        ViewpointPtr location;
        //! Agent's current camera heading in radians
        double heading = 0;
        //! Agent's current camera elevation in radians
        double elevation = 0;
        //! Vector of nearby navigable locations representing action candidates
        std::vector<ViewpointPtr> navigableLocations;
    };

    typedef std::shared_ptr<SimState> SimStatePtr;

    /**
     * Class for representing nearby candidate locations that can be moved to.
     */
    struct Location {
        bool included;
        std::string viewpointId;
        glm::mat4 pose;
        glm::vec3 pos;
        std::vector<bool> unobstructed;
        GLuint cubemap_texture;
    };

    typedef std::shared_ptr<Location> LocationPtr;

    /**
     * Main class for accessing an instance of the simulator environment.
     */
    class Simulator {
    public:
        Simulator();
                      
        ~Simulator();

        /**
         * Sets camera resolution. Default is 320 x 240.
         */
        void setCameraResolution(int width, int height);
        
        /**
         * Sets camera vertical field-of-view in degrees. Default is 45 degrees.
         */
        void setCameraFOV(double vfov);
        
        /**
         * Initialize the simulator. Further camera configuration won't take any effect from now on.
         */
        void init();
        
        /**
         * Set a non-standard path to the <a href="https://niessner.github.io/Matterport/">Matterport3D dataset</a>. 
         * The provided directory must contain subdirectories of the form: 
         * "/v1/scans/<scanId>/matterport_skybox_images/". Default is "./data" (expected location of dataset symlink).
         */
        void setDatasetPath(const std::string& path);
        
        /**
         * Set a non-standard path to the viewpoint connectivity graphs. The provided directory must contain subdirectories
         * of the form "/<scanId>_connectivity.json". Default is "./connectivity" (the graphs provided
         * by this repo).
         */
        void setNavGraphPath(const std::string& path);
        
        /**
         * Set the random seed for episodes where viewpoint is not provided.
         */
        void setSeed(int seed) { generator.seed(seed); };
        
        /**
         * Set the camera elevation min and max limits in radians. Default is +-0.94 radians.
         * @return true if successful.
         */
        bool setElevationLimits(double min, double max);
        
        /**
         * Starts a new episode. If a viewpoint is not provided initialization will be random.
         * @param scanId - sets which scene is used, e.g. "2t7WUuJeko7"
         * @param viewpointId - sets the initial viewpoint location, e.g. "cc34e9176bfe47ebb23c58c165203134"
         * @param heading - set the agent's initial heading in radians
         * @param elevation - set the initial camera elevation in radians
         */
        void newEpisode(const std::string& scanId, const std::string& viewpointId=std::string(), 
              double heading=0, double elevation=0);
        
        /**
         * Returns the current environment state including RGB image and available actions.
         */
        SimStatePtr getState();
        
        /** @brief Select an action.
         *
         * An RL agent will sample an action here. A task-specific reward can be determined 
         * based on the location, heading, elevation, etc. of the resulting state.
         * @param index - an index into the set of feasible actions defined by getState()->navigableLocations.
         * @param heading - desired heading change in radians
         * @param elevation - desired elevation change in radians
         */
        void makeAction(int index, double heading, double elevation);
        
        /**
         * Closes the environment and releases underlying texture resources, OpenGL contexts, etc.
         */
        void close();
    private:
        void loadLocationGraph();
        void clearLocationGraph();
        void populateNavigable();
        void loadTexture(int locationId);
        void setHeading(double heading);
        void setElevation(double elevation);
        void renderScene();
#ifdef OSMESA_RENDERING
        void *buffer;
        OSMesaContext ctx;
#else
        GLuint FramebufferName;
#endif
        SimStatePtr state;
        bool initialized;
        int width;
        int height;
        double vfov;
        double minElevation;
        double maxElevation;
        glm::mat4 Projection;
        glm::mat4 View;
        glm::mat4 Model;
        GLint PVM;
        GLint vertex;
        GLuint ibo_cube_indices;
        GLuint vbo_cube_vertices;
        GLuint glProgram;
        GLuint glShaderV;
        GLuint glShaderF;
        std::string datasetPath;
        std::string navGraphPath;
        std::vector<LocationPtr> locations;
        std::default_random_engine generator;
        Timer cpuLoadTimer;
        Timer gpuLoadTimer;
        Timer renderTimer;
        Timer totalTimer;
    };
}

#endif
