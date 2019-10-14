#include <iostream>
#include <opencv2/opencv.hpp>

#include "MatterSim.hpp"

using namespace mattersim;

#define WIDTH  1280
#define HEIGHT 720

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

int main(int argc, char *argv[]) {

    cv::namedWindow("C++ RGB");
    cv::namedWindow("C++ Depth");

    Simulator sim;

    // Sets resolution. Default is 320X240
    sim.setCameraResolution(640,480);
    sim.setDepthEnabled(false);

    // Initialize the simulator. Further camera configuration won't take any effect from now on.
    sim.initialize();

    std::cout << "\nC++ Demo" << std::endl;
    std::cout << "Showing some random viewpoints in one building." << std::endl;

    int i = 0;
    while(true) {
        i++;
        std::cout << "Episode #" << i << "\n";

        // Starts a new episode. It is not needed right after init() but it doesn't cost much and the loop is nicer.
        sim.newRandomEpisode(std::vector<std::string>(1,"pa4otMbVnkk")); // Launches at a random location

        for (int k=0; k<500; k++) {

            // Get the state
            SimStatePtr state = sim.getState().at(0); // SimStatePtr is std::shared_ptr<SimState>

            // Which consists of:
            unsigned int n = state->step;
            cv::Mat rgb  = state->rgb; // OpenCV CV_8UC3 type (i.e. 8bit color rgb)
            cv::Mat depth  = state->depth; // OpenCV CV_16UC1 type (i.e. 16bit grayscale)
            ViewpointPtr location = state->location; // Need a class to hold viewpoint id, and x,y,z location of a viewpoint
            float heading = state->heading;
            float elevation = state->elevation; // camera parameters
            std::vector<ViewpointPtr> reachable = state->navigableLocations; // Where we can move to,
            int locationIdx = 0; // Must be an index into reachable
            double headingChange = M_PI / 500;
            double elevationChange = 0;

            cv::imshow("C++ RGB", rgb);
            cv::imshow("C++ Depth", depth);
            cv::waitKey(10);

            sim.makeAction(std::vector<unsigned int>(1, locationIdx), 
                           std::vector<double>(1, headingChange), 
                           std::vector<double>(1, elevationChange));

        }
    }

    // It will be done automatically in destructor but after close you can init it again with different settings.
    sim.close();

    return 0;
}
