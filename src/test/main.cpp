#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <string>
#include <cstdlib>
#include <ctime>

#include <json/json.h>
#include <opencv2/opencv.hpp>

#include "Catch.hpp"
#include "MatterSim.hpp"


using namespace mattersim;

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

// Note: tests tagged with 'Rendering' will require the dataset to be installed
// To run tests without the dataset installed:
//      $ ./build/tests exclude:[Rendering]


double radians(double deg) {
    return deg * M_PI / 180.0;
}

double degrees(double rad) {
    return (rad * 180.0) / M_PI;
}

float heading[10] =           {  10,  350, 350,   1,  90, 180,   90,  270,   90, 270 };
float heading_chg[10] =       { -20, -360, 371,  89,  90, -90, -180, -180, -180,   0 };
float discreteHeading[10] =   {   0,  330, 300, 330,   0,  30,    0,  330,  300, 270 };
float elevation[10] =         {  10,   10, -26, -40, -40, -40,   50,   50,   40,   0 };
float elevation_chg[10] =     {   0,  -36, -30, -10,   0,  90,    5,  -10,  -40,   0 };
float discreteElevation[10] = {   0,    0, -30, -30, -30, -30,    0,   30,    0, -30 };
unsigned int viewIndex[10] =  {  12,   23,  10,  11,   0,   1,   12,   35,   22,   9 };


TEST_CASE( "Continuous Motion", "[Actions]" ) {

    std::vector<std::string> scanIds {"2t7WUuJeko7", "17DRP5sb8fy"};
    std::vector<std::string> viewpointIds {"cc34e9176bfe47ebb23c58c165203134", "5b9b2794954e4694a45fc424a8643081"};
    Simulator sim;
    sim.setCameraResolution(200,100); // width,height
    sim.setCameraVFOV(radians(45)); // 45deg vfov, 90deg hfov
    sim.setRenderingEnabled(false);
    sim.setBatchSize(scanIds.size());
    CHECK(sim.setElevationLimits(radians(-40),radians(50)));
    REQUIRE_NOTHROW(sim.initialize());
    std::vector<double> headings(scanIds.size(),radians(heading[0]));
    std::vector<double> elevations(scanIds.size(),radians(elevation[0]));
    REQUIRE_NOTHROW(sim.newEpisode(scanIds, viewpointIds, headings, elevations));
    std::vector<unsigned int> ix;
    for (int t = 0; t < 10; ++t ) {
        headings.clear();
        elevations.clear();
        for (int i = 0; i < scanIds.size(); ++i) {
            INFO("i=" << i << ", t=" << t);
            std::string scanId = scanIds[i];
            std::string viewpointId = viewpointIds.at(i);
            SimStatePtr state = sim.getState().at(i);
            CHECK( state->scanId == scanId );
            CHECK( state->step == t );
            CHECK( state->heading == Approx(radians(heading[t])) );
            CHECK( state->elevation == Approx(radians(elevation[t])) );
            CHECK( state->rgb.rows == 100 );
            CHECK( state->rgb.cols == 200 );
            CHECK( state->location->viewpointId == viewpointId );
            CHECK( state->viewIndex == 0 ); // not active
            std::vector<ViewpointPtr> actions = state->navigableLocations;
            ix.push_back(t % actions.size()); // select an action
            headings.push_back(radians(heading_chg[t]));
            elevations.push_back(radians(elevation_chg[t]));
        }
        sim.makeAction(ix, headings, elevations);
    }
    REQUIRE_NOTHROW(sim.close());
}


TEST_CASE( "Discrete Motion", "[Actions]" ) {

    std::vector<std::string> scanIds {"2t7WUuJeko7", "17DRP5sb8fy"};
    std::vector<std::string> viewpointIds {"cc34e9176bfe47ebb23c58c165203134", "5b9b2794954e4694a45fc424a8643081"};
    Simulator sim;
    sim.setCameraResolution(200,100); // width,height
    sim.setCameraVFOV(radians(45)); // 45deg vfov, 90deg hfov
    sim.setRenderingEnabled(false);
    sim.setBatchSize(scanIds.size());
    sim.setDiscretizedViewingAngles(true);
    CHECK(sim.setElevationLimits(radians(-10),radians(10))); // should be disregarded
    REQUIRE_NOTHROW(sim.initialize());
    std::vector<double> headings(scanIds.size(),radians(heading[0]));
    std::vector<double> elevations(scanIds.size(),radians(elevation[0]));
    REQUIRE_NOTHROW(sim.newEpisode(scanIds, viewpointIds, headings, elevations));
    std::vector<unsigned int> ix;
    for (int t = 0; t < 10; ++t ) {
        headings.clear();
        elevations.clear();
        for (int i = 0; i < scanIds.size(); ++i) {
            INFO("i=" << i << ", t=" << t);
            std::string scanId = scanIds[i];
            std::string viewpointId = viewpointIds.at(i);
            SimStatePtr state = sim.getState().at(i);
            CHECK( state->scanId == scanId );
            CHECK( state->step == t );
            CHECK( state->heading == Approx(radians(discreteHeading[t])) );
            CHECK( state->elevation == Approx(radians(discreteElevation[t])) );
            CHECK( state->rgb.rows == 100 );
            CHECK( state->rgb.cols == 200 );
            CHECK( state->location->viewpointId == viewpointId );
            CHECK( state->viewIndex == viewIndex[t] );
            std::vector<ViewpointPtr> actions = state->navigableLocations;
            ix.push_back(t % actions.size()); // select an action
            headings.push_back(radians(heading_chg[t]));
            elevations.push_back(radians(elevation_chg[t]));
        }
        sim.makeAction(ix, headings, elevations);
    }
    REQUIRE_NOTHROW(sim.close());
}


TEST_CASE( "Robot Relative Coords", "[Actions]" ) {

    std::vector<std::string> scanIds {"2t7WUuJeko7", "17DRP5sb8fy"};
    std::vector<std::string> viewpointIds {"cc34e9176bfe47ebb23c58c165203134", "5b9b2794954e4694a45fc424a8643081"};
    Simulator sim;
    sim.setCameraResolution(200,100); // width,height
    sim.setCameraVFOV(radians(45)); // 45deg vfov, 90deg hfov
    sim.setRenderingEnabled(false);
    sim.setBatchSize(scanIds.size());
    CHECK(sim.setElevationLimits(radians(-40),radians(50)));
    REQUIRE_NOTHROW(sim.initialize());
    std::vector<double> headings(scanIds.size(),radians(heading[0]));
    std::vector<double> elevations(scanIds.size(),radians(elevation[0]));
    REQUIRE_NOTHROW(sim.newEpisode(scanIds, viewpointIds, headings, elevations));
    std::vector<unsigned int> ix;
    for (int t = 0; t < 10; ++t ) {
        headings.clear();
        elevations.clear();
        for (int i = 0; i < scanIds.size(); ++i) {
            INFO("i=" << i << ", t=" << t);
            SimStatePtr state = sim.getState().at(i);
            cv::Point3f curr(state->location->x, state->location->y, state->location->z);
            std::vector<ViewpointPtr> actions = state->navigableLocations;
            double last_angle = 0.0;
            int k = 0;
            for (auto loc: actions ){
                if (k == 0) {
                    CHECK(state->location->rel_heading == Approx(0));
                    CHECK(state->location->rel_elevation == Approx(0));
                    CHECK(state->location->rel_distance == Approx(0));
                    k++;
                    continue;
                }
                double curr_angle = sqrt(loc->rel_heading*loc->rel_heading + loc->rel_elevation*loc->rel_elevation);
                // Should be getting further from the centre of the image
                CHECK(curr_angle >= last_angle);
                last_angle = curr_angle;
                // Robot rel coordinates should describe the position
                double h = state->heading + loc->rel_heading;
                double e = state->elevation + loc->rel_elevation;
                INFO("k="<< k << ", heading=" << degrees(state->heading) << ", rel_heading=" << degrees(loc->rel_heading));
                INFO("elevation=" << degrees(state->elevation) << ", rel_elevation=" << degrees(loc->rel_elevation));
                INFO("rel_distance=" << loc->rel_distance);
                INFO("curr=(" << curr.x << ", " << curr.y << ", " << curr.z << ")");
                INFO("targ=(" << loc->x << ", " << loc->y << ", " << loc->z << ")"); 
                INFO("diff=(" << loc->x-curr.x << ", " << loc->y-curr.y << ", " << loc->z-curr.z << ")"); 
                cv::Point3f offset(sin(h)*cos(e)*loc->rel_distance, cos(h)*cos(e)*loc->rel_distance, sin(e)*loc->rel_distance);
                INFO("calc diff=(" << offset.x << ", " << offset.y << ", " << offset.z << ")"); 
                cv::Point3f target = curr + offset;
                REQUIRE(loc->x == Approx(target.x));
                REQUIRE(loc->y == Approx(target.y));
                REQUIRE(loc->z == Approx(target.z));
                k++;
            }
            ix.push_back(t % actions.size()); // select an action
            headings.push_back(radians(heading_chg[t]));
            elevations.push_back(radians(elevation_chg[t]));
        }
        sim.makeAction(ix, headings, elevations);
    }
    REQUIRE_NOTHROW(sim.close());
}


TEST_CASE( "Navigable Locations", "[Actions]" ) {

    std::vector<std::string> scanIds;
    std::ifstream infile ("./connectivity/scans.txt", std::ios_base::in);
    std::string scanId;
    while (infile >> scanId) {
        scanIds.push_back (scanId);
    }
    Simulator sim;
    sim.setCameraResolution(20,20); // don't really care about the image
    sim.setCameraVFOV(radians(90)); // 90deg vfov, 90deg hfov
    double half_hfov = M_PI/4;
    sim.setRenderingEnabled(false);
    sim.setSeed(1);
    sim.setBatchSize(scanIds.size());
    REQUIRE_NOTHROW(sim.initialize());
    std::vector<double> headings;
    std::vector<double> elevations;
    std::vector<unsigned int> ix;
    REQUIRE_NOTHROW(sim.newRandomEpisode(scanIds)); // start anywhere, but repeatably so

    // Load connectivity graphs
    std::vector<Json::Value> graphs;
    for (auto scanId : scanIds) {
        Json::Value root;
        auto navGraphFile =  "./connectivity/" + scanId + "_connectivity.json";
        std::ifstream ifs(navGraphFile, std::ifstream::in);
        ifs >> root;
        graphs.push_back(root);
    }

    // Check a short episode in each scan
    for (int t = 0; t < 10; ++t ) {

        for (int k = 0; k < scanIds.size(); ++k) {
            auto scanId = scanIds.at(k);
            Json::Value root = graphs.at(k);

            SimStatePtr state = sim.getState().at(k);
            CHECK( state->scanId == scanId );
            CHECK( state->step == t );

            // Check included viewpoints
            std::vector<bool> included;
            for (auto viewpoint : root) {
                included.push_back(viewpoint["included"].asBool());
                if (viewpoint["image_id"].asString() == state->location->viewpointId) {
                    INFO("Don't let simulator navigate to an excluded viewpoint");
                    CHECK(included.back());
                }
            }

            // Store navigableLocations from sim into a map
            std::unordered_map <std::string, ViewpointPtr> locs;
            for (auto v : state->navigableLocations) {
                locs[v->viewpointId] = v;
            }

            // Check current viewpoint exists in json file
            Json::Value currentViewpoint;
            for (auto viewpoint : root) {
                auto viewpointId = viewpoint["image_id"].asString();
                if (viewpointId == state->location->viewpointId) {
                    currentViewpoint = viewpoint;
                    break;
                }
            }
            REQUIRE_FALSE(currentViewpoint.empty());

            // Check navigableLocations are correct
            int navigableCount = 0;
            for (int i = 0; i < included.size(); ++i) {
                std::string curr = currentViewpoint["image_id"].asString();
                std::string target = root[i]["image_id"].asString();
                // Read current position
                float x = currentViewpoint["pose"][3].asFloat();
                float y = currentViewpoint["pose"][7].asFloat();
                float z = currentViewpoint["pose"][11].asFloat();
                // Read target position
                float tar_x = root[i]["pose"][3].asFloat();
                float tar_y = root[i]["pose"][7].asFloat();
                float tar_z = root[i]["pose"][11].asFloat();
                if (curr == target) {
                    INFO("Viewpoint " << target << " must be self-reachable");
                    // Every viewpoint must be self reachable
                    REQUIRE(locs.find(target) != locs.end());
                    // We should never be at a not included viewpoint
                    CHECK(included[i]);
                    ViewpointPtr target_viewpoint = locs[target];
                    CHECK(target_viewpoint->x == Approx(tar_x));
                    CHECK(target_viewpoint->y == Approx(tar_y));
                    CHECK(target_viewpoint->z == Approx(tar_z));
                    navigableCount++;
                } else if (!currentViewpoint["unobstructed"][i].asBool()) {
                    // obstructed
                    INFO("Viewpoint " << target << " is obstructed from "
                          << curr << ", can't be a navigableLocation");
                    CHECK(locs.find(target) == locs.end());
                } else if (!included[i]) {
                    INFO("Viewpoint " << target << " is excluded,"
                          << " can't be a navigableLocation");
                    CHECK(locs.find(target) == locs.end());
                } else {
                    // check if this viewpoint is visible
                    INFO("atan2 " << atan2(tar_y - y, tar_x - x));
                    float viewpointHeading = M_PI/2 - atan2(tar_y - y, tar_x - x);
                    // convert interval [-0.5pi, 1.5pi] to interval [0, 2pi]
                    if (viewpointHeading < 0) {
                        viewpointHeading += 2*M_PI;
                    }
                    bool visible = fabs(state->heading - viewpointHeading) <= half_hfov ||
                          fabs(state->heading + 2.0 * M_PI - viewpointHeading) <= half_hfov ||
                          fabs(state->heading - (viewpointHeading + 2.0 * M_PI)) <= half_hfov;
                    INFO("Estimated heading " << viewpointHeading << ", agent heading " << state->heading
                        << ", visible " << visible);
                    if (visible) {
                        INFO("Viewpoint " << target << " (" << tar_x << ", " << tar_y << ", " << tar_z 
                            << ") should be reachable from "  << curr << " (" << x << ", " << y << ", " << z 
                            << ") with heading " << state->heading);
                        REQUIRE(locs.find(target) != locs.end());
                        ViewpointPtr target_viewpoint = locs[target];
                        CHECK(target_viewpoint->x == Approx(tar_x));
                        CHECK(target_viewpoint->y == Approx(tar_y));
                        CHECK(target_viewpoint->z == Approx(tar_z));
                        navigableCount++;
                    } else {
                        INFO("Viewpoint " << target << " (" << tar_x << ", " << tar_y << ", " << tar_z 
                            << ") is not visible in camera from "  << curr << " (" << x << ", " << y << ", " << z 
                            << ") with heading " << state->heading << ", can't be a navigableLocation");
                        REQUIRE(locs.find(target) == locs.end());
                    }
                }
            }
            CHECK(navigableCount == state->navigableLocations.size());

            std::vector<ViewpointPtr> actions = state->navigableLocations;
            ix.push_back(t % actions.size()); // select an action
            headings.push_back(radians(heading_chg[t]));
            elevations.push_back(radians(elevation_chg[t]));
        }
        sim.makeAction(ix, headings, elevations);
    }
    REQUIRE_NOTHROW(sim.close());
}


TEST_CASE( "RGB Image", "[Rendering]" ) {

    Simulator sim;
    sim.setCameraResolution(640,480); // width,height
    sim.setCameraVFOV(radians(60)); // 60deg vfov, 80deg hfov
    CHECK(sim.setElevationLimits(radians(-40),radians(50)));
    unsigned int batchSize = 4;
    sim.setBatchSize(batchSize);
    REQUIRE_NOTHROW(sim.initialize());
    Json::Value root;
    std::string testSpecFile{"src/test/rendertest_spec.json"};
    std::ifstream ifs(testSpecFile, std::ifstream::in);
    if (ifs.fail()){
        throw std::invalid_argument( "Could not open test spec file: " + testSpecFile );
    }
    ifs >> root;

    std::vector<std::string> scanIds(batchSize);
    std::vector<std::string> viewpointIds(batchSize);
    std::vector<double> headings(batchSize);
    std::vector<double> elevations(batchSize);

    for (auto testbatch : root) {
        for (unsigned int n=0; n<batchSize; ++n) {
            auto testcase = testbatch[n];
            scanIds.at(n) = testcase["scanId"].asString();
            viewpointIds.at(n) = testcase["viewpointId"].asString();
            headings.at(n) = testcase["heading"].asFloat();
            elevations.at(n) = testcase["elevation"].asFloat();
        }
        INFO(testbatch);
        REQUIRE_NOTHROW(sim.newEpisode(scanIds, viewpointIds, headings, elevations));
        for (unsigned int n=0; n<batchSize; ++n) {
            auto testcase = testbatch[n];
            auto imgfile = testcase["reference_image"].asString();
            auto reference_image = cv::imread("webgl_imgs/"+imgfile);
            auto state = sim.getState().at(n);
            double err = cv::norm(reference_image, state->rgb, cv::NORM_L2);
            err /= reference_image.rows * reference_image.cols;
            CHECK(err < 0.15);
            // save for later comparison, these images can also be inspected by hand
            cv::imwrite("sim_imgs/"+imgfile, state->rgb);
        }
    }
    REQUIRE_NOTHROW(sim.close());
}


TEST_CASE( "Timing", "[Rendering]" ) {

    // Initialize random generator
    std::srand(std::time(NULL));

    // Load environment names
    std::vector<std::string> envs;
    std::ifstream is("connectivity/scans.txt");
    std::string line;
    while (std::getline(is, line)) {
        envs.push_back(line);
    }

    Simulator sim;
    sim.setCameraResolution(640,480); // width,height
    sim.setCameraVFOV(radians(60)); // 60deg vfov, 80deg hfov
    sim.setRenderingEnabled(true);
    sim.setDiscretizedViewingAngles(true);
    sim.setPreloadingEnabled(true);
    int batchSize = envs.size();
    sim.setBatchSize(batchSize);
    sim.setDepthEnabled(false);
    REQUIRE_NOTHROW(sim.initialize());

    std::vector<unsigned int> ix(batchSize);
    std::vector<double> headings(batchSize);
    std::vector<double> elevations(batchSize);
    std::vector<SimStatePtr> state;

    for (int i=0; i<10; i++) {
        std::random_shuffle(envs.begin(), envs.end());
        REQUIRE_NOTHROW(sim.newRandomEpisode(envs));
        state = sim.getState();
        for (int j=1; j<50; j++) {
            for (int n=0; n<batchSize; ++n) {
                ix.at(n) = std::rand() % state.at(n)->navigableLocations.size();
                headings.at(n) = (std::rand() % 3) - 1;
                elevations.at(n) = (std::rand() % 3) - 1;
            }
            sim.makeAction(ix, headings, elevations);
            state = sim.getState();
        }
    }
    std::cout << sim.timingInfo() << std::endl;
    REQUIRE_NOTHROW(sim.close());
}
