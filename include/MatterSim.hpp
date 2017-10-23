#ifndef MATTERSIM_HPP
#define MATTERSIM_HPP

#include <memory>
#include <vector>

#include <opencv2/opencv.hpp>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace mattersim {
    struct Viewpoint {
        unsigned int id;
        cv::Point3f location;
    };

    typedef std::shared_ptr<Viewpoint> ViewpointPtr;

    /**
     * Simulator state class.
     */
    struct SimState {
        //! Number of frames since the last newEpisode() call
        unsigned int step;
        //! RGB image taken from the agent's current viewpoint
        cv::Mat rgb;
        //! Depth image taken from the agent's current viewpoint (not implemented)
        cv::Mat depth;
        //! Agent's current 3D location
        ViewpointPtr location;
        //! Agent's current camera heading in radians
        float heading;
        //! Agent's current camera elevation in radians
        float elevation;
        //! Vector of nearby navigable locations representing action candidates
        std::vector<ViewpointPtr> navigableLocations;
    };

    typedef std::shared_ptr<SimState> SimStatePtr;

    /**
     * Class for representing nearby candidate locations that can be moved to.
     */
    struct Location {
        bool included;
        std::string image_id;
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
        Simulator() : state{new SimState()}, width(320), height(240), navGraphPath("./connectivity") {};
        
        /**
         * Set path to the <a href="https://niessner.github.io/Matterport/">Matterport3D dataset</a>. 
         * The provided directory must contain subdirectories of the form: 
         * "/v1/scans/<scanId>/matterport_skybox_images/".
         */
        void setDatasetPath(std::string path);
        
        /**
         * Sets path to viewpoint connectivity graphs. The provided directory must contain subdirectories
         * of the form "/<scanId>_connectivity.json". Default is "./connectivity" (the graphs provided
         * by this repo).
         */
        void setNavGraphPath(std::string path);
        
        /**
         * Sets resolution. Default is 320 x 240.
         */
        void setScreenResolution(int width, int height);
        
        /**
         * Sets which scene is used, e.g. "2t7WUuJeko7".
         */
        void setScanId(std::string id);
        
        /**
         * Initialize the simulator. Further configuration won't take any effect from now on.
         */
        void init();
        
        /**
         * Starts a new episode. It is not needed right after init().
         */
        void newEpisode();
        
        /**
         * Returns true after the end episode action has been selected by the agent 
         * (after which no more actions can be made).
         */
        bool isEpisodeFinished();
        
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
        void makeAction(int index, float heading, float elevation);
        
        /**
         * Closes the environment and releases underlying texture resources, OpenGL contexts, etc.
         */
        void close();
    private:
        void populateNavigable();
        SimStatePtr state;
        int width;
        int height;
        glm::mat4 Projection;
        glm::mat4 View;
        glm::mat4 Model;
        GLint PVM;
        GLint vertex;
        GLuint FramebufferName;
        std::string datasetPath;
        std::string navGraphPath;
        std::string scanId;
        std::vector<LocationPtr> locations;
    };
}

#endif
