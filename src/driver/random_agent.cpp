#include <iostream>
#include <random>
#include <thread>
#include <chrono>

#include <opencv2/opencv.hpp>

#include "MatterSim.hpp"

using namespace mattersim;

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

int main(int argc, char *argv[]) {
//    std::mt19937 random_source(0);
    std::minstd_rand random_source(0);
    std::uniform_real_distribution<float> angledist(-M_PI / 4, M_PI / 4);
    Simulator *sim = new Simulator();

    // Sets resolution. Default is 320X240
    sim->setCameraResolution(640,480);

    // Initialize the simulator. Further camera configuration won't take any effect from now on.
    sim->init();

    // Run this many episodes
    int episodes = 10;

    for (int i = 0; i < episodes; ++i) {

        // Starts a new episode. It is not needed right after init() but it doesn't cost much and the loop is nicer.
        sim->newEpisode("2t7WUuJeko7"); // Take optional viewpoint_id argument, otherwise launches at a random location

        for (int step = 0;step < 10;++step) {
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
            std::uniform_int_distribution<> distribution(0, reachable.size() - 1);
            int locationIdx = distribution(random_source);
            float headingChange = angledist(random_source);
            float elevationChange = angledist(random_source);

/*            std::stringstream ss;
            ss << "rgb_" << i << "_" << step << ".png";
            cv::imwrite(ss.str(), rgb);*/

            // Make action (index into reachable, heading change in rad, elevation change in rad)      
// E.g. an RL agent will sample an action here. A reward can be determined based on location, heading, elevation but that is dataset dependent
            sim->makeAction(locationIdx, headingChange, elevationChange);
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
        }
    }

    // It will be done automatically in destructor but after close You can init it again with different settings.
    sim->close();
    delete sim;

    return 0;
}
