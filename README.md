# Matterport3D Simulator
AI Research Platform for Reinforcement Learning from Real Panoramic Images.

The Matterport3D Simulator enables development of AI **agents that interact with real 3D environments using visual information** (RGB-D images). It is primarily intended for research in deep reinforcement learning, at the intersection of computer vision, natural language processing and robotics.

## Features
- Dataset consisting of 90 different predominantly indoor environments,
- All images are real, not synthetic (providing much more visual complexity),
- API for C++ and Python
- Customizable image resolution, camera parameters, etc,
- Supports GPU rendering using OpenGL, as well as off-screen CPU rendering using MESA,
- Future releases will include depth data (RGB-D).

## Cite as

Todo

### Bibtex:
```
todo
```

## Dataset

Matterport3D Simulator is based on densely sampled 360-degree indoor RGB-D images from the [Matterport3D dataset](https://niessner.github.io/Matterport/). The dataset consists of 90 different indoor environments, including homes, offices, churches and hotels. Each environment contains full 360-degree RGB-D scans from between 8 and 349 viewpoints, spread on average 2.25m apart throughout the entire walkable floorplan of the scene. 

### Actions

At each viewpoint location, the agent can pan and elevate the camera. The agent can also choose to move between viewpoints. The precise details of the agent's observations and actions are configurable.

## Installation/Building instructions

### Prerequisites

Matterport3D Simulator has several dependencies:
- [OpenCV](http://opencv.org/) >= 2.4 including 3.0 
- [OpenGL](https://www.opengl.org/)
- [OSMesa](https://www.mesa3d.org/osmesa.html)
- [GLM](https://glm.g-truc.net/0.9.8/index.html)
- [Numpy](http://www.numpy.org/)
- [pybind11](https://github.com/pybind/pybind11) for Python bindings
- [Doxygen](http://www.doxygen.org) for building documentation

E.g. installing dependencies on Ubuntu:
```
sudo apt-get install libopencv-dev python-opencv freeglut3 freeglut3-dev libglm-dev libjsoncpp-dev doxygen libosmesa6-dev libosmesa6
```

### Clone Repo

Clone the Matterport3DSimulator repository:
```
# Make sure to clone with --recursive
git clone --recursive https://github.com/peteanderson80/Matterport3DSimulator.git
cd Matterport3DSimulator
```

If you didn't clone with the `--recursive` flag, then you'll need to manually clone the pybind submodule from the top-level directory:
```
git submodule update --init --recursive
```

### Dataset Download

Download the Matterport3D dataset which is available after requesting access [here](https://niessner.github.io/Matterport/). The provided download script allows for downloading of selected data types. Note that for the Matterport3D Simulator, only the following data types are required:
- matterport_skybox_images

Create a symlink to the Matterport3D Dataset, where <Matterdata> should be a directory structured as ```<Matterdata>/v1/scans/<scanId>/matterport_skybox_images/*.jpg```:
```
ln -s <Matterdata> data
```

Using symlinks will allow the same Matterport3D dataset installation to be used between multiple projects.

### Compiling

Build OpenGL version using CMake:
```
mkdir build && cd build
cmake ..
make
cd ../
```

Or build headless OSMESA version using CMake:
```
mkdir build && cd build
cmake -DOSMESA_RENDERING=ON ..
make
cd ../
```

To build html docs for C++ classes in the `doxygen` directory, just run:
```
doxygen
```

### Demo

```
build/mattersim_main
```

### Running Tests

```
build/tests
```

Or if you haven't downloaded and linked the dataset yet:
```
build/tests exclude:[Rendering]
```

Refer to the [Catch](https://github.com/philsquared/Catch) documentation for additional usage and configuration options.

