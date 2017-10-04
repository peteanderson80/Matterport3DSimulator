.PHONY: all pybind

all: lib/MatterSim.o bin/mattersim_main

bin/mattersim_main: lib/MatterSim.o src/driver/mattersim_main.cpp
	@mkdir -p bin
	g++ -std=c++11 src/driver/mattersim_main.cpp -Iinclude lib/MatterSim.o -o bin/mattersim_main -L/usr/lib -lOpenGL -lGLEW -lopencv_core -lopencv_imgcodecs -lopencv_highgui

lib/MatterSim.o: include/MatterSim.hpp src/lib/MatterSim.cpp
	@mkdir -p lib
	g++ -std=c++11 src/lib/MatterSim.cpp -fPIC -c -o lib/MatterSim.o -Iinclude

pybind: lib/MatterSim.o
	@mkdir -p lib
	g++ -shared -std=c++11 -fPIC -Iinclude -I`python3 -c "import numpy;print(numpy.get_include())"` `python3 -m pybind11 --includes` src/lib_python/MatterSimPython.cpp lib/MatterSim.o -o lib/MatterSim`python3-config --extension-suffix` -lOpenGL -lGLEW -lopencv_core -lopencv_highgui

clean:
	@rm -f lib/* bin/*
