#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "Catch.hpp"
#include "MatterSim.hpp"


using namespace mattersim;

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

// Dataset must be installed to run tests


double radians(float deg) {
    return deg * M_PI / 180.f;
}

TEST_CASE( "Simulator can start new episodes and do simple motion", "[Simulator]" ) {

    std::string scanId = "2t7WUuJeko7";
    std::string viewpointId = "cc34e9176bfe47ebb23c58c165203134";
    float heading[10] =     {  10,  350, 350,  1, 90, 180,   90,  270,   90, 270 }; 
    float heading_chg[10] = { -20, -360, 371, 89, 90, -90, -180, -180, -180,   0 };
    float elevation[10] =     {  10,   10, -26, -40, -40, -40,  50,  50,  40,  0 };
    float elevation_chg[10] = {   0,  -36, -30, -10,   0,  90,   5, -10, -40,  0 };
    Simulator sim;
    sim.setCameraResolution(200,100); // width,height
    sim.setCameraFOV(45); // 45deg vfov, 90deg hfov
    CHECK(sim.setElevationLimits(radians(-40),radians(50)));
    REQUIRE_NOTHROW(sim.init());
    REQUIRE_NOTHROW(sim.newEpisode(scanId, viewpointId, radians(heading[0]), radians(elevation[0])));
    
    for (int t = 0; t < 10; ++t ) {
        SimStatePtr state = sim.getState();
        CHECK( state->scanId == scanId );
        CHECK( state->step == t );
        CHECK( state->heading == Approx(radians(heading[t])) );
        CHECK( state->elevation == Approx(radians(elevation[t])) );
        CHECK( state->rgb.rows == 100 );
        CHECK( state->rgb.cols == 200 );
        CHECK( state->location->viewpointId == viewpointId );
        std::vector<ViewpointPtr> actions = state->navigableLocations;
        int ix = t % actions.size(); // select an action
        sim.makeAction(ix, radians(heading_chg[t]), radians(elevation_chg[t]));
        viewpointId = actions[ix]->viewpointId;
    }
    REQUIRE_NOTHROW(sim.close());
}

// TODO - Test Simulator state->navigableLocations is correct - check valid and invalid actions (check camera angle boundaries, and 'not included' viewpoints)
// TODO - Test Simulator state->rgb is correct - even after switching lots of different episodes on the same simulator


