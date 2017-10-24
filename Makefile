CONFIG_FILE := Makefile.config
# Check for the config file.
ifeq ($(wildcard $(CONFIG_FILE)),)
	$(error $(CONFIG_FILE) not found. See $(CONFIG_FILE).example.)
endif
include $(CONFIG_FILE)

LIBRARIES += GL GLEW opencv_core opencv_highgui jsoncpp
ifeq ($(OPENCV_VERSION), 3)
	LIBRARIES += opencv_imgcodecs
endif
LDFLAGS += $(foreach library,$(LIBRARIES),-l$(library))

ifeq ($(PYTHON_VERSION), 3)
	PYTHON += python3
else
	PYTHON += python
endif

.PHONY: all pybind

all: lib/MatterSim.o lib/Benchmark.o bin/mattersim_main bin/random_agent

bin/mattersim_main: lib/MatterSim.o lib/Benchmark.o src/driver/mattersim_main.cpp
	@mkdir -p bin
	g++ -std=c++11 src/driver/mattersim_main.cpp -Iinclude lib/MatterSim.o lib/Benchmark.o -o bin/mattersim_main -L/usr/lib $(LDFLAGS)

bin/random_agent: lib/MatterSim.o lib/Benchmark.o src/driver/random_agent.cpp
	@mkdir -p bin
	g++ -std=c++11 src/driver/random_agent.cpp -Iinclude lib/MatterSim.o lib/Benchmark.o -o bin/random_agent -L/usr/lib $(LDFLAGS)

lib/MatterSim.o: include/MatterSim.hpp src/lib/MatterSim.cpp
	@mkdir -p lib
	g++ -std=c++11 src/lib/MatterSim.cpp -fPIC -c -o lib/MatterSim.o -Iinclude

lib/Benchmark.o: include/Benchmark.hpp src/lib/Benchmark.cpp
	@mkdir -p lib
	g++ -std=c++11 src/lib/Benchmark.cpp -fPIC -c -o lib/Benchmark.o -Iinclude

pybind: lib/MatterSim.o
	@mkdir -p lib
	g++ -shared -std=c++11 -fPIC -Iinclude -I`$(PYTHON) -c "import numpy;print(numpy.get_include())"` `$(PYTHON) -m pybind11 --includes` `$(PYTHON)-config --includes` src/lib_python/MatterSimPython.cpp lib/MatterSim.o lib/Benchmark.o -o lib/MatterSim`$(PYTHON)-config --extension-suffix` $(LDFLAGS)

clean:
	@rm -f lib/* bin/*

docs:
	doxygen
