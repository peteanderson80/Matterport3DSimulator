#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "Catch.hpp"
#include "MatterSim.hpp"


using namespace mattersim;

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif


TEST_CASE( "Factorials are computed", "[factorial]" ) {

    Simulator sim;
    sim.init();
}

TEST_CASE( "vectors can be sized and resized", "[vector]" ) {

    std::vector<int> v( 5 );
    
    REQUIRE( v.size() == 5 );
    REQUIRE( v.capacity() >= 5 );
    
    SECTION( "resizing bigger changes size and capacity" ) {
        v.resize( 10 );
        
        REQUIRE( v.size() == 10 );
        REQUIRE( v.capacity() >= 10 );
    }
    SECTION( "resizing smaller changes size but not capacity" ) {
        v.resize( 0 );
        
        REQUIRE( v.size() == 0 );
        REQUIRE( v.capacity() >= 5 );
    }
    SECTION( "reserving bigger changes capacity but not size" ) {
        v.reserve( 10 );
        
        REQUIRE( v.size() == 5 );
        REQUIRE( v.capacity() >= 10 );
    }
    SECTION( "reserving smaller does not change size or capacity" ) {
        v.reserve( 0 );
        
        REQUIRE( v.size() == 5 );
        REQUIRE( v.capacity() >= 5 );
    }
}