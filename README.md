# Matterport3D Simulator
AI Research Platform for Reinforcement Learning from Real Panoramic Images.

The Matterport3D Simulator enables development of AI **agents that interact with real 3D environments using visual information** (RGB-D images). It is primarily intended for research in deep reinforcement learning, at the intersection of computer vision, natural language processing and robotics.

![Concept](teaser.jpg)

*This is development code for early release. We may make breaking changes, particularly as we look at possible integration with [ParlAI](https://github.com/facebookresearch/ParlAI) and [OpenAI Gym](https://github.com/openai/gym).*

## Features
- Dataset consisting of 90 different predominantly indoor environments,
- All images are real, not synthetic (providing much more visual complexity),
- API for C++ and Python
- Customizable image resolution, camera parameters, etc,
- Supports GPU rendering using OpenGL, as well as off-screen CPU rendering using OSMESA,
- Future releases will include depth data (RGB-D) as well as class and instance object segmentations.

## Reference

The Matterport3D Simulator and the Room-to-Room (R2R) navigation dataset are described in:
- [Vision-and-Language Navigation: Interpreting visually-grounded navigation instructions in real environments](https://arxiv.org/abs/1711.07280). 

If you use the simulator or dataset, please cite our paper:

### Bibtex:
```
@article{mattersim,
  title={{Vision-and-Language Navigation}: Interpreting visually-grounded navigation instructions in real environments},
  author={Peter Anderson and Qi Wu and Damien Teney and Jake Bruce and Mark Johnson and Niko Sünderhauf and Ian Reid and Stephen Gould and Anton van den Hengel},
  journal={arXiv preprint arXiv:1711.07280},
  year={2017}
}
```

## Simulator Data

Matterport3D Simulator is based on densely sampled 360-degree indoor RGB-D images from the [Matterport3D dataset](https://niessner.github.io/Matterport/). The dataset consists of 90 different indoor environments, including homes, offices, churches and hotels. Each environment contains full 360-degree RGB-D scans from between 8 and 349 viewpoints, spread on average 2.25m apart throughout the entire walkable floorplan of the scene. 

### Actions

At each viewpoint location, the agent can pan and elevate the camera. The agent can also choose to move between viewpoints. The precise details of the agent's observations and actions are described in the paper and defined in `include/MatterSim.hpp`.

## Tasks

Currently the simulator supports one task. We hope this will grow.

### Room-to-Room (R2R) Navigation Task

Please refer to [specific instructions](tasks/R2R/README.md) to setup and run this task.

## Installation / Build Instructions

### Prerequisites

A C++ compiler with C++11 support is required. Matterport3D Simulator has several dependencies:
- [OpenCV](http://opencv.org/) >= 2.4 including 3.x 
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

### Directory Structure

- `connectivity`: Json navigation graphs.
- `webgl_imgs`: Contains dataset views rendered with javascript (for test comparisons).
- `sim_imgs`: Will contain simulator rendered images after running tests.
- `models`: Caffe models for precomputing ResNet image features.
- `img_features`: Storage for precomputed image features.
- `data`: You create a symlink to the Matterport3D dataset.
- `tasks`: Currently just the Room-to-Room (R2R) navigation task.

Other directories are mostly self-explanatory.

### Dataset Download

To use the simulator you must first download **either** the [Matterport3D Dataset](https://niessner.github.io/Matterport/), **or** you can download the precomputed ResNet image features and use discretized viewpoints.

#### Matterport3D Dataset

Download the Matterport3D dataset which is available after requesting access [here](https://niessner.github.io/Matterport/). The provided download script allows for downloading of selected data types. Note that for the Matterport3D Simulator, only the following data types are required (and can be selected with the download script):
- `matterport_skybox_images`

Create a symlink to the Matterport3D Dataset, which should be structured as ```<Matterdata>/v1/scans/<scanId>/matterport_skybox_images/*.jpg```:
```
ln -s <Matterdata> data
```

Using symlinks will allow the same Matterport3D dataset installation to be used between multiple projects.

#### Precomputing ResNet Image Features

To speed up model training times, it is convenient to discretize heading and elevation into 30 degree increments, and to precompute image features for each view. 

We generate image features using Caffe. To replicate our approach, first download and save some Caffe ResNet-152 weights into the `models` directory. We experiment with weights pretrained on [ImageNet](https://github.com/KaimingHe/deep-residual-networks), and also weights finetuned on the [Places365](https://github.com/CSAILVision/places365) dataset. The script `scripts/precompute_features.py` can then be used to precompute ResNet-152 features. Features are saved in tsv format in the `img_features` directory. 

Alternatively, skip the generation and just download and extract our tsv files into the `img_features` directory:
- [ResNet-152-imagenet features [380K/2.9GB]](https://storage.googleapis.com/bringmeaspoon/img_features/ResNet-152-imagenet.zip)
- [ResNet-152-places365 features [380K/2.9GB]](https://storage.googleapis.com/bringmeaspoon/img_features/ResNet-152-places365.zip)

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
To build html docs for C++ classes in the `doxygen` directory, run this command and navigate to `doxygen/html/index.html`:
```
doxygen
```

### Demo

These are very simple demos designed to illustrate the use of the simulator in python and C++. Use the arrow keys to pan and tilt the camera. In the python demo, the top row number keys can be used to move to another viewpoint (if any are visible).

Python demo:
```
python src/driver/driver.py
```
C++ demo:
```
build/mattersim_main
```

### Running Tests
```
build/tests
```
Or, if you haven't installed the Matterport3D dataset, you will need to skip the rendering tests:
```
build/tests exclude:[Rendering]
```
Refer to the [Catch](https://github.com/philsquared/Catch) documentation for additional usage and configuration options.


## License

The Matterport3D dataset, and data derived from it, is released under the [Matterport3D Terms of Use](http://dovahkiin.stanford.edu/matterport/public/MP_TOS.pdf). Our code is released under the MIT license.

## Acknowledgements

We would like to thank Matterport for allowing the Matterport3D dataset to be used by the academic community. This project is supported by a Facebook ParlAI Research Award and by the [Australian Centre for Robotic Vision](https://www.roboticvision.org/). 

## Contributing

We welcome contributions from the community. All submissions require review and in most cases would require tests.




