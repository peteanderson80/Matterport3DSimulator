# Matterport3D Simulator
AI Research Platform for Reinforcement Learning from Real Panoramic Images.

The Matterport3D Simulator enables development of AI **agents that interact with real 3D environments using visual information** (RGB-D images). It is primarily intended for research in deep reinforcement learning, at the intersection of computer vision, natural language processing and robotics.

## Features (eventually maybe)
- Dataset consisting of 90 different predominantly indoor environments,
- All images are real, not synthetic (providing much more visual complexity),
- API for C++ and Python
- Customizable resolution, camera parameters, etc,
- GPU or CPU (off-screen) rendering,
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
- [GLM](https://glm.g-truc.net/0.9.8/index.html)
- [Numpy](http://www.numpy.org/)
- [pybind11](https://github.com/pybind/pybind11)

E.g. installing dependencies on Ubuntu:
```
sudo apt-get install libopencv-dev python-opencv freeglut3 freeglut3-dev libglm-dev libjsoncpp-dev
```

### Compiling

Similar to [Caffe](http://caffe.berkeleyvision.org/installation.html), configure the build by copying and modifying the example Makefile.config for your setup. Uncomment the relevant lines if using OpenCV >= 3 or Python 3.

```
cp Makefile.config.example Makefile.config
# Adjust Makefile.config (for example, if using OpenCV >= 3 or Python 3)
make
make pybind
```

## Demo


