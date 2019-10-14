project(Matterport_Simulator CXX)
cmake_minimum_required(VERSION 2.8)

option(OSMESA_RENDERING "Offscreen CPU rendering with OSMesa" OFF)
option(EGL_RENDERING "Offscreen GPU rendering with EGL" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
# Make custom find-modules available
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include_directories("${PROJECT_SOURCE_DIR}/include")

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenMP)
if (OPENMP_CXX_FOUND)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

pkg_check_modules(JSONCPP REQUIRED jsoncpp)

if(EGL_RENDERING)
  add_definitions(-DEGL_RENDERING)
  find_package(OpenGL REQUIRED COMPONENTS OpenGL EGL)
  pkg_check_modules(EPOXY REQUIRED epoxy)
  set(GL_LIBS OpenGL::OpenGL OpenGL::EGL ${EPOXY_LIBRARIES})
elseif(OSMESA_RENDERING)
  add_definitions(-DOSMESA_RENDERING)
  pkg_check_modules(OSMESA REQUIRED osmesa)
  set(GL_LIBS ${OSMESA_LIBRARIES})
else()
  cmake_policy(SET CMP0072 OLD)
  find_package(OpenGL REQUIRED)
  find_package(GLEW REQUIRED)
  set(GL_LIBS ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})
endif()

add_library(MatterSim SHARED src/lib/MatterSim.cpp src/lib/NavGraph.cpp src/lib/Benchmark.cpp src/lib/cbf.cpp)
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

add_subdirectory(pybind11)

find_package(PythonInterp 3)
message(${PYTHON_EXECUTABLE})

# Need to search for python executable again to pick up an activated
# virtualenv python, if any.
unset(PYTHON_EXECUTABLE CACHE)
find_program(PYTHON_EXECUTABLE python
      PATHS ENV PATH         # look in the PATH environment variable
      NO_DEFAULT_PATH        # do not look anywhere else...
      )

find_package(NumPy REQUIRED)

pybind11_add_module(MatterSimPython src/lib_python/MatterSimPython.cpp)
target_include_directories(MatterSimPython PRIVATE ${NUMPY_INCLUDES})
target_link_libraries(MatterSimPython PRIVATE MatterSim)
set_target_properties(MatterSimPython
  PROPERTIES
  OUTPUT_NAME MatterSim)
