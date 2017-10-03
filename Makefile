all: lib/MatterSim.o
	@mkdir -p bin
	g++ src/driver/mattersim_main.cpp -Iinclude lib/MatterSim.o -o bin/mattersim_main -L/usr/lib -lOpenGL -lGLEW -lopencv_core -lopencv_imgcodecs -lopencv_highgui

lib/MatterSim.o: include/MatterSim.hpp src/lib/MatterSim.cpp
	@mkdir -p lib
	g++ src/lib/MatterSim.cpp -c -o lib/MatterSim.o -Iinclude

clean:
	@rm -f lib/* bin/*
