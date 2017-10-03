#ifndef MATTERSIM_HPP
#define MATTERSIM_HPP

#include <memory>
#include <vector>

#include <opencv/cv.h>

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

    struct SimState {
        unsigned int step;
        cv::Mat rgb;
        cv::Mat depth;
        ViewpointPtr location;
        float heading;
        float elevation;
        std::vector<ViewpointPtr> navigableLocations;
    };

    typedef std::shared_ptr<SimState> SimStatePtr;

    class Simulator {
    public:
        Simulator() : state{new SimState()} {};
        void setDatasetPath(std::string path);
        void setNavGraphPath(std::string path);
        void setScreenResolution(int width, int height);
        void setScanId(std::string id);
        void init();
        void newEpisode();
        bool isEpisodeFinished();
        SimStatePtr getState();
        void makeAction(int index, float heading, float elevation);
        void close();
    private:
        SimStatePtr state;
        int width;
        int height;
        glm::mat4 Projection;
        glm::mat4 View;
        glm::mat4 Model;
        GLint PVM;
        GLint vertex;
    };
}

#endif
