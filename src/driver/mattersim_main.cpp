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
    cv::namedWindow("displaywin");

    Simulator *sim = new Simulator();

    // Sets resolution. Default is 320X240
    sim->setCameraResolution(640,480);

    // Initialize the simulator. Further camera configuration won't take any effect from now on.
    sim->init();

    // Run this many episodes
    int episodes = 10;

    for (int i = 0; i < episodes; ++i) {
        std::cout << "Episode #" << i + 1 << "\n";

        // Starts a new episode. It is not needed right after init() but it doesn't cost much and the loop is nicer.
        sim->newEpisode("2t7WUuJeko7"); // Take optional viewpoint_id argument, otherwise launches at a random location

        while (true) {

            // Get the state
            SimStatePtr state = sim->getState(); // SimStatePtr is std::shared_ptr<SimState>

            // Which consists of:
            unsigned int n = state->step;
            cv::Mat rgb  = state->rgb; // Use OpenCV CV_8UC3 type (i.e. 8bit color rgb)
            cv::Mat depth  = state->depth; // ignore for now
            ViewpointPtr location = state->location; // Need a class to hold viewpoint id, and cv::Point3 for x,y,z location of a viewpoint
            float heading = state->heading;
            float elevation = state->elevation; // camera parameters
            std::vector<ViewpointPtr> reachable = state->navigableLocations; // Where we can move to,
            int locationIdx = 0;
            float headingChange = 0;
            float elevationChange = 0;

            cv::imshow("displaywin", rgb);
            // Make action (index into reachable, heading change in rad, elevation change in rad)      
// E.g. an RL agent will sample an action here. A reward can be determined based on location, heading, elevation but that is dataset dependent
            int key = cv::waitKey(1);

            switch (key) {
            case -1:
                continue;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                locationIdx = key - '0';
                if (locationIdx >= reachable.size()) {
                    locationIdx = 0;
                }
                break;
            case 'q':
                return 0;
                break;
            case 81:
                headingChange = -M_PI / 180;
                break;
            case 82:
                elevationChange = M_PI / 180;
                break;
            case 83:
                headingChange = M_PI / 180;
                break;
            case 84:
                elevationChange = -M_PI / 180;
                break;
            }
            sim->makeAction(locationIdx, headingChange, elevationChange);

        }
        std::cout << "Episode finished.\n";
    }

    // It will be done automatically in destructor but after close You can init it again with different settings.
    sim->close();
    delete sim;

    return 0;
}
