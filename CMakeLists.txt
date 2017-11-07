project(Matterport_Simulator CXX)
cmake_minimum_required(VERSION 2.8)

option(OSMESA_RENDERING "Render offscreen with OSMesa" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include_directories("${PROJECT_SOURCE_DIR}/include")

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(JSONCPP REQUIRED jsoncpp)

if(OSMESA_RENDERING)
  pkg_check_modules(OSMESA REQUIRED osmesa)
  set(GL_LIBS ${OSMESA_LIBRARIES})
else()
  find_package(OpenGL REQUIRED)
  find_package(GLEW REQUIRED)
  set(GL_LIBS ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})
endif()

add_library(MatterSim SHARED src/lib/MatterSim.cpp src/lib/Benchmark.cpp)
if(OSMESA_RENDERING)
  target_compile_definitions(MatterSim PUBLIC "-DOSMESA_RENDERING")
endif()
target_include_directories(MatterSim PRIVATE ${JSONCPP_INCLUDE_DIRS})
target_link_libraries(MatterSim ${JSONCPP_LIBRARIES} ${OpenCV_LIBS} ${GL_LIBS})

add_executable(tests src/test/main.cpp)
target_include_directories(tests PRIVATE ${JSONCPP_INCLUDE_DIRS})
target_link_libraries(tests MatterSim ${JSONCPP_LIBRARIES} ${OpenCV_LIBS})

add_executable(mattersim_main src/driver/mattersim_main.cpp)
target_link_libraries(mattersim_main MatterSim)

add_executable(random_agent src/driver/random_agent.cpp)
target_link_libraries(random_agent MatterSim)

add_subdirectory(pybind11)

find_package(PythonInterp 2.7)
message(${PYTHON_EXECUTABLE})

# Need to search for python executable again to pick up an activated
# virtualenv python, if any.
unset(PYTHON_EXECUTABLE CACHE)
find_program(PYTHON_EXECUTABLE python
      PATHS ENV PATH         # look in the PATH environment variable
      NO_DEFAULT_PATH        # do not look anywhere else...
      )
# Make FindNumPy available
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
find_package(NumPy REQUIRED)

pybind11_add_module(MatterSimPython src/lib_python/MatterSimPython.cpp)
target_include_directories(MatterSimPython PRIVATE ${NUMPY_INCLUDES})
target_link_libraries(MatterSimPython PRIVATE MatterSim)
set_target_properties(MatterSimPython
  PROPERTIES
  OUTPUT_NAME MatterSim)
