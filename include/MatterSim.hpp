#ifndef MATTERSIM_HPP
#define MATTERSIM_HPP

#include <memory>
#include <vector>
#include <random>
#include <cmath>
#include <stdexcept>

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
#include "glm/ext.hpp"

#include "Benchmark.hpp"
#include "NavGraph.hpp"

namespace mattersim {

    struct Viewpoint: std::enable_shared_from_this<Viewpoint> {
        Viewpoint(std::string viewpointId, unsigned int ix, double x, double y, double z,
          double rel_heading, double rel_elevation, double rel_distance) : 
            viewpointId(viewpointId), ix(ix), x(x), y(y), z(z), rel_heading(rel_heading),
            rel_elevation(rel_elevation), rel_distance(rel_distance)  
        {}

        //! Viewpoint identifier
        std::string viewpointId;
        //! Viewpoint index into connectivity graph
        unsigned int ix;
        //! 3D position in world coordinates
        double x;
        double y;
        double z;
        //! Heading relative to the camera
        double rel_heading;
        //! Elevation relative to the camera
        double rel_elevation;
        //! Distance from the agent
        double rel_distance;
    };

    typedef std::shared_ptr<Viewpoint> ViewpointPtr;
    struct ViewpointPtrComp {
        inline bool operator() (const ViewpointPtr& l, const ViewpointPtr& r){
            return sqrt(l->rel_heading*l->rel_heading+l->rel_elevation*l->rel_elevation)
                < sqrt(r->rel_heading*r->rel_heading+r->rel_elevation*r->rel_elevation);
        }
    };

    /**
     * Simulator state class.
     */
    struct SimState: std::enable_shared_from_this<SimState>{
        //! Building / scan environment identifier
        std::string scanId;
        //! Number of frames since the last newEpisode() call
        unsigned int step = 0;
        //! RGB image (in BGR channel order) from the agent's current viewpoint
        cv::Mat rgb;
        //! Depth image taken from the agent's current viewpoint
        cv::Mat depth;
        //! Agent's current 3D location
        ViewpointPtr location;
        //! Agent's current camera heading in radians
        double heading = 0;
        //! Agent's current camera elevation in radians
        double elevation = 0;
        //! Agent's current view [0-35] (set only when viewing angles are discretized)
        //! [0-11] looking down, [12-23] looking at horizon, [24-35] looking up
        unsigned int viewIndex = 0;
        //! Vector of nearby navigable locations representing state-dependent action candidates, i.e.
        //! viewpoints you can move to. Index 0 is always to remain at the current viewpoint.
        //! The remaining viewpoints are sorted by their angular distance from the centre of the image.
        std::vector<ViewpointPtr> navigableLocations;
    };

    typedef std::shared_ptr<SimState> SimStatePtr;


    /**
     * Main class for accessing an instance of the simulator environment.
     */
    class Simulator {

    public:
        Simulator();

        ~Simulator();

        /**
         * Set a non-standard path to the <a href="https://niessner.github.io/Matterport/">Matterport3D dataset</a>.
         * The provided directory must contain subdirectories of the form:
         * "<scanId>/matterport_skybox_images/". Default is "./data/v1/scans/".
         */
        void setDatasetPath(const std::string& path);

        /**
         * Set a non-standard path to the viewpoint connectivity graphs. The provided directory must contain files
         * of the form "/<scanId>_connectivity.json". Default is "./connectivity" (the graphs provided
         * by this repo).
         */
        void setNavGraphPath(const std::string& path);

        /**
         * Enable or disable rendering. Useful for testing. Default is true (enabled).
         */
        void setRenderingEnabled(bool value);

        /**
         * Sets camera resolution. Default is 320 x 240.
         */
        void setCameraResolution(int width, int height);

        /**
         * Sets camera vertical field-of-view in radians. Default is 0.8, approx 46 degrees.
         */
        void setCameraVFOV(double vfov);

        /**
         * Set the camera elevation min and max limits in radians. Default is +-0.94 radians.
         * @return true if successful.
         */
        bool setElevationLimits(double min, double max);

        /**
         * Enable or disable discretized viewing angles. When enabled, heading and
         * elevation changes will be restricted to 30 degree increments from zero,
         * with left/right/up/down movement triggered by the sign of the makeAction
         * heading and elevation parameters. Default is false (disabled).
         */
        void setDiscretizedViewingAngles(bool value);

        /**
         * Enable or disable restricted navigation. When enabled, the navigable locations
         * that the agent can move to are restricted to nearby viewpoints that are in 
         * the camera field of view given the current heading. When disabled, the agent 
         * can always move to any adjacent viewpoint in the navigation graph. 
         * Default is true (enabled).
         */
        void setRestrictedNavigation(bool value);

        /**
         * Enable or disable preloading of images from disk to CPU memory. Default is false (disabled).
         * Enabled is better for training models, but will cause a delay when starting the simulator.
         */
        void setPreloadingEnabled(bool value);

        /**
         * Enable or disable rendering of depth images. Default is false (disabled).
         */
        void setDepthEnabled(bool value);

        /**
         * Set the number of environments in the batch. Default is 1.
         */
        void setBatchSize(unsigned int size);

        /**
         * Set the cache size for storing pano images in gpu memory. Default is 200. Should be comfortably
         * larger than the batch size.
         */
        void setCacheSize(unsigned int size);

        /**
         * Set the random seed for episodes where viewpoint is not provided.
         */
        void setSeed(int seed);

        /**
         * Initialize the simulator. Further configuration won't take any effect from now on.
         */
        void initialize();

        /**
         * Starts a new episode. If a viewpoint is not provided initialization will be random.
         * @param scanId - sets which scene is used, e.g. "2t7WUuJeko7"
         * @param viewpointId - sets the initial viewpoint location, e.g. "cc34e9176bfe47ebb23c58c165203134"
         * @param heading - set the agent's initial camera heading in radians. With z-axis up,
         *                  heading is defined relative to the y-axis (turning right is positive).
         * @param elevation - set the initial camera elevation in radians, measured from the horizon
         *                    defined by the x-y plane (up is positive).
         */
        void newEpisode(const std::vector<std::string>& scanId, const std::vector<std::string>& viewpointId,
              const std::vector<double>& heading, const std::vector<double>& elevation);

        /**
         * Starts a new episode at a random viewpoint.
         * @param scanId - sets which scene is used, e.g. "2t7WUuJeko7" 
         */
        void newRandomEpisode(const std::vector<std::string>& scanId);

        /**
         * Returns the current batch of environment states including RGB images and available actions.
         */
        const std::vector<SimStatePtr>& getState();

        /** @brief Select an action.
         *
         * An RL agent will sample an action here. A task-specific reward can be determined
         * based on the location, heading, elevation, etc. of the resulting state.
         * @param index - an index into the set of feasible actions defined by getState()->navigableLocations.
         * @param heading - desired heading change in radians. With z-axis up, heading is defined
         *                  relative to the y-axis (turning right is positive).
         * @param elevation - desired elevation change in radians, measured from the horizon defined
         *                    by the x-y plane (up is positive).
         */
        void makeAction(const std::vector<unsigned int>& index, const std::vector<double>& heading, 
                        const std::vector<double>& elevation);

        /**
         * Closes the environment and releases underlying texture resources, OpenGL contexts, etc.
         */
        void close();

        /**
         * Reset the rendering timers that run automatically.
         */
        void resetTimers();

        /**
         * Return a formatted timing string.
         */
        std::string timingInfo(); 

    private:
        const int headingCount = 12; // 12 heading values in discretized views
        const double elevationIncrement = M_PI/6.0; // 30 degrees discretized up/down
        void populateNavigable();
        void setHeadingElevation(const std::vector<double>& heading, const std::vector<double>& elevation);
        void renderScene();
#ifdef OSMESA_RENDERING
        void *buffer;
        OSMesaContext ctx;
#elif defined (EGL_RENDERING)
        EGLDisplay eglDpy;
        GLuint FramebufferName;
#else
        GLuint FramebufferName;
#endif
        std::vector<SimStatePtr> states;
        bool initialized;
        bool renderingEnabled;
        bool discretizeViews;
        bool restrictedNavigation;
        bool preloadImages;
        bool renderDepth;
        int width;
        int height;
        int randomSeed;
        unsigned int cacheSize;
        unsigned int batchSize;
        double vfov;
        double minElevation;
        double maxElevation;
        glm::mat4 Projection;
        glm::mat4 View;
        glm::mat4 Model;
        glm::mat4 Scale;
        glm::mat4 RotateX;
        glm::mat4 RotateZ;
        GLint ProjMat;
        GLint ModelViewMat;
        GLint vertex;
        GLint isDepth;
        GLuint vao_cube;
        GLuint vbo_cube_vertices;
        GLuint glProgram;
        GLuint glShaderV;
        GLuint glShaderF;
        std::string datasetPath;
        std::string navGraphPath;
        Timer preloadTimer; // Preloading images from disk into cpu memory
        Timer loadTimer; // Loading textures from disk or cpu memory onto gpu
        Timer renderTimer; // Rendering time
        Timer gpuReadTimer; // Reading rendered images from gpu back to cpu memory
        Timer processTimer; // Total run time for simulator
        Timer wallTimer; // Wall clock timer
        unsigned int frames;
    };
}

#endif
