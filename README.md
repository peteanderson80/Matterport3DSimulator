# Matterport3D Simulator
AI Research Platform for Reinforcement Learning from Real Panoramic Images.

The Matterport3D Simulator enables development of AI **agents that interact with real 3D environments using visual information** (RGB-D images). It is primarily intended for research in deep reinforcement learning, at the intersection of computer vision, natural language processing and robotics.

![Concept](teaser.jpg)

Visit the main [website](https://bringmeaspoon.org/) for updates and to view a demo.

## Features
- Dataset consisting of 90 different predominantly indoor environments,
- All images are real, not synthetic (providing much more visual complexity),
- API for C++ and Python
- Outputs RGB and depth images
- Customizable image resolution, camera parameters, etc,
- Supports off-screen rending (both GPU and CPU based)
- Fast (xxx fps RGB-D off-screen rendering at 640x480 resolution using a Titan X GPU)
- Future releases may support class and instance object segmentations.

## Reference

The Matterport3D Simulator and the Room-to-Room (R2R) navigation dataset are described in:
- [Vision-and-Language Navigation: Interpreting visually-grounded navigation instructions in real environments](https://arxiv.org/abs/1711.07280).

If you use the simulator or dataset, please cite our paper (CVPR 2018 spotlight oral):

### Bibtex:
```
@inproceedings{mattersim,
  title={{Vision-and-Language Navigation}: Interpreting visually-grounded navigation instructions in real environments},
  author={Peter Anderson and Qi Wu and Damien Teney and Jake Bruce and Mark Johnson and Niko S{\"u}nderhauf and Ian Reid and Stephen Gould and Anton van den Hengel},
  booktitle={Proceedings of the IEEE Conference on Computer Vision and Pattern Recognition (CVPR)},
  year={2018}
}
```

## Simulator Data

Matterport3D Simulator is based on densely sampled 360-degree indoor RGB-D images from the [Matterport3D dataset](https://niessner.github.io/Matterport/). The dataset consists of 90 different indoor environments, including homes, offices, churches and hotels. Each environment contains full 360-degree RGB-D scans from between 8 and 349 viewpoints, spread on average 2.25m apart throughout the entire walkable floorplan of the scene.

### Actions

At each viewpoint location, the agent can pan and elevate the camera. The agent can also choose to move between viewpoints. The precise details of the agent's observations and actions are described in the paper and defined in `include/MatterSim.hpp`.

### Room-to-Room (R2R) Navigation Task

The simulator includes the training data and evaluation metrics for the Room-to-Room (R2R) Navigation task, which requires an autonomous agent to follow a natural language navigation instruction to navigate to a goal location in a previously unseen building. Please refer to [specific instructions](tasks/R2R/README.md) to setup and run this task. There is a test server and leaderboard available at [EvalAI](https://evalai.cloudcv.org/web/challenges/challenge-page/97/overview).

## Installation / Build Instructions

We recommend using our docker image to install the simulator. The simulator can also be built from source but satisfying the project dependencies may be more difficult.

### Prerequisites

- Ubuntu 16.04
- Nvidia GPU with driver >= 384
- Install [docker](https://docs.docker.com/engine/installation/)
- Install [nvidia-docker2.0](https://github.com/nvidia/nvidia-docker/wiki/Installation-(version-2.0))
- Note: CUDA / CuDNN toolkits do not need to be installed (these are provided by the docker image)

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

To use the simulator you must first download the [Matterport3D Dataset](https://niessner.github.io/Matterport/) which is available after requesting access [here](https://niessner.github.io/Matterport/). The download script that will be provided allows for downloading of selected data types. To run the simulator, only the `matterport_skybox_images` are needed.

Set an environment variable to the location of the dataset, where <PATH> is the full absolute path (not a relative path or symlink) to the directory containing the individual matterport scan directories (17DRP5sb8fy, 2t7WUuJeko7, etc):
```
export MATTERPORT_DATA_DIR=<PATH>
```

Note that if <PATH> is a remote sshfs mount, you will need to mount it with the `-o allow_root` option or the docker container won't be able to access this directory. 


### Building and Testing using Docker

Build the docker image:
```
docker build -t mattersim .
```

Run the docker container, mounting both the git repo and the dataset:
```
nvidia-docker run -it --mount type=bind,source=$MATTERPORT_DATA_DIR,target=/root/mount/Matterport3DSimulator/data/v1/scans,readonly --volume `pwd`:/root/mount/Matterport3DSimulator mattersim
```

Now (from inside the docker container), build the simulator and run the unit tests:
```
cd /root/mount/Matterport3DSimulator
mkdir build && cd build
cmake -DEGL_RENDERING=ON ..
make
cd ../
./build/tests
```

Assuming all tests pass, some information about the rendering frame rate will be printed to stdout and `sim_imgs` will now contain some test images rendered by the simulator. 


### Rendering Options (GPU, CPU, off-screen)

There are three rendering options, which are selected using [cmake](https://cmake.org/) options during the build process (by varying line 3 in the build commands immediately above):
- GPU rendering using OpenGL (requires an X server): `cmake ..` (default)
- Off-screen GPU rendering using [EGL](https://www.khronos.org/egl/): `cmake -DEGL_RENDERING=ON ..`
- Off-screen CPU rendering using [OSMesa](https://www.mesa3d.org/osmesa.html): `cmake -DOSMESA_RENDERING=ON ..`

The recommended (fast) approach for training agents is using off-screen GPU rendering (EGL).


### Interactive Demo

To run an interactive demo, build the docker image as described above (`docker build -t mattersim .`), then run the docker container while sharing the host's X server and DISPLAY environment variable with the container:
```
xhost +
nvidia-docker run -it -e DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix --mount type=bind,source=$MATTERPORT_DATA_DIR,target=/root/mount/Matterport3DSimulator/data/v1/scans,readonly --volume `pwd`:/root/mount/Matterport3DSimulator mattersim
cd /root/mount/Matterport3DSimulator
```

Be sure to build the simulator using the cmake option for on-screen rendering (`cmake ..`). Commands for running both python and C++ demos are provided below. These are very simple demos designed to illustrate the use of the simulator in python and C++. Use the arrow keys to pan and tilt the camera. In the python demo, the top row number keys can be used to move to another viewpoint (if any are visible).

Python demo:
```
python src/driver/driver.py
```
C++ demo:
```
build/mattersim_main
```


### Building without Docker

The simulator can be built outside of a docker container using the cmake build commands described above. However, this is not the recommended approach, as all dependencies will need to be installed locally and may conflict with existing libraries. The main requirements are:
- Ubuntu >= 14.04
- Nvidia-driver with CUDA installed 
- C++ compiler with C++11 support
- [CMake](https://cmake.org/) >= 3.10
- [OpenCV](http://opencv.org/) >= 2.4 including 3.x
- [OpenGL](https://www.opengl.org/)
- [GLM](https://glm.g-truc.net/0.9.8/index.html)
- [Numpy](http://www.numpy.org/)
Optional dependences (depending on the cmake rendering options):
- [OSMesa](https://www.mesa3d.org/osmesa.html) for OSMesa backend support
- [epoxy](https://github.com/anholt/libepoxy) for EGL backend support

The provided [Dockerfile](Dockerfile) contains install commands for most of these libraries. For example, to install OpenGL and related libraries:
```
sudo apt-get install libjsoncpp-dev libepoxy-dev libglm-dev libosmesa6 libosmesa6-dev libglew-dev
```


### Simulator API

TODO

To build html docs for C++ classes in the `doxygen` directory, run this command and navigate to `doxygen/html/index.html`:
```
doxygen
```


### Directory Structure

- `connectivity`: Json navigation graphs.
- `webgl_imgs`: Contains dataset views rendered with javascript (for test comparisons).
- `sim_imgs`: Will contain simulator rendered images after running tests.
- `models`: Caffe models for precomputing ResNet image features.
- `img_features`: Storage for precomputed image features.
- `data`: Matterport3D dataset.
- `tasks`: Currently just the Room-to-Room (R2R) navigation task.

Other directories are mostly self-explanatory.


## License

The Matterport3D dataset, and data derived from it, is released under the [Matterport3D Terms of Use](http://dovahkiin.stanford.edu/matterport/public/MP_TOS.pdf). Our code is released under the MIT license.

## Acknowledgements

We would like to thank Matterport for allowing the Matterport3D dataset to be used by the academic community. This project is supported by a Facebook ParlAI Research Award and by the [Australian Centre for Robotic Vision](https://www.roboticvision.org/).

## Contributing

We welcome contributions from the community. All submissions require review and in most cases would require tests.
