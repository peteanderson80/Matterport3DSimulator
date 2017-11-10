#ifndef MATTERSIM_HPP
#define MATTERSIM_HPP

#include <memory>
#include <vector>
#include <random>
#include <time.h>
#include <cmath>

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
        //! Viewpoint index into connectivity graph
        unsigned int ix;
        //! 3D position in world coordinates
        cv::Point3f point;
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
     * Internal class for representing nearby candidate locations that can be moved to.
     */
    struct Location {
        //! True if viewpoint is included in the simulator. Sometimes duplicated viewpoints have been excluded.
        bool included;
        //! Unique Matterport identifier for every pano location
        std::string viewpointId;
        //! Rotation component
        glm::mat4 rot;
        //! Translation component
        glm::vec3 pos;
        std::vector<bool> unobstructed;
        GLuint cubemap_texture;
    };

    typedef std::shared_ptr<Location> LocationPtr;

    /**
     * Main class for accessing an instance of the simulator environment.
     */
    class Simulator {
        friend class SimulatorPython;
    public:
        Simulator();
                      
        ~Simulator();

        /**
         * Sets camera resolution. Default is 320 x 240.
         */
        void setCameraResolution(int width, int height);
        
        /**
         * Sets camera vertical field-of-view in radians. Default is 0.8, approx 46 degrees.
         */
        void setCameraVFOV(double vfov);
        
        /**
         * Enable or disable rendering. Useful for testing. Default is true (enabled).
         */
        void setRenderingEnabled(bool value);

        /**
         * Enable or disable discretized viewing angles. When enabled, heading and 
         * elevation changes will be restricted to 30 degree increments from zero, 
         * with left/right/up/down movement triggered by the sign of the makeAction 
         * heading and elevation parameters. Default is false (disabled).
         */
        void setDiscretizedViewingAngles(bool value);

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
         * Set a non-standard path to the viewpoint connectivity graphs. The provided directory must contain files
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
         * @param heading - set the agent's initial camera heading in radians. With z-axis up, 
         *                  heading is defined relative to the y-axis (turning right is positive).
         * @param elevation - set the initial camera elevation in radians, measured from the horizon 
         *                    defined by the x-y plane (up is positive).
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
         * @param heading - desired heading change in radians. With z-axis up, heading is defined 
         *                  relative to the y-axis (turning right is positive).
         * @param elevation - desired elevation change in radians, measured from the horizon defined 
         *                    by the x-y plane (up is positive).
         */
        void makeAction(int index, double heading, double elevation);
        
        /**
         * Closes the environment and releases underlying texture resources, OpenGL contexts, etc.
         */
        void close();
    private:
        const int headingCount = 12; // 12 heading values in discretized views
        const double elevationIncrement = M_PI/6.0; // 30 degrees discretized up/down
        void loadLocationGraph();
        void clearLocationGraph();
        void populateNavigable();
        void loadTexture(int locationId);
        void setHeadingElevation(double heading, double elevation);
        void renderScene();
#ifdef OSMESA_RENDERING
        void *buffer;
        OSMesaContext ctx;
#else
        GLuint FramebufferName;
#endif
        SimStatePtr state;
        bool initialized;
        bool renderingEnabled;
        bool discretizeViews;
        int width;
        int height;
        double vfov;
        double minElevation;
        double maxElevation;
        glm::mat4 Projection;
        glm::mat4 View;
        glm::mat4 Model;
        glm::mat4 Scale;
        glm::mat4 RotateX;
        glm::mat4 RotateZ;
        GLint PVM;
        GLint vertex;
        GLuint ibo_cube_indices;
        GLuint vbo_cube_vertices;
        GLuint glProgram;
        GLuint glShaderV;
        GLuint glShaderF;
        std::string datasetPath;
        std::string navGraphPath;
        std::map<std::string, std::vector<LocationPtr> > scanLocations;
        std::default_random_engine generator;
        Timer cpuLoadTimer;
        Timer gpuLoadTimer;
        Timer renderTimer;
        Timer totalTimer;
    };
}

#endif
