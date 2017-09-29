# Matterport3DSimulator
AI Research Platform for Reinforcement Learning from Real Panoramic Images.

This simulator enables development of AI **agents that interact with real 3D environments using visual information** (RGB-D images). It is primarily intended for research in deep reinforcement learning, at the intersection of computer vision, natural language processing and robotics.

## Features (eventually maybe)
- Dataset consisting of 90 different predominantly indoor environments,
- All images are real, not synthetic (providing much more visual complexity),
- API for C++ and Python
- Fast (up to xx fps using GPU),
- Customizable resolution, camera parameters, and actions,
- Includes depth data (RGB-D),
- Off-screen rendering,
- Multi-platform.

## Cite as

Todo

### Bibtex:
```
todo
```

## Dataset

Matterport3D Simulator is based on densely sampled 360-degree indoor RGB-D images from the [Matterport3D dataset](https://niessner.github.io/Matterport/). The dataset consists of 90 different indoor environments, including homes, offices, churches and hotels. Each environment contains full 360-degree RGB-D scans from between 8 and 349 viewpoints, spread on average 2.25m apart throughout the entire walkable floorplan of the scene. 

### Actions

At each viewpoint location, the agent can pan and tilt the camera. The agent can also choose to move between viewpoints. The precise details of the agent's observations and actions are configurable.

## Installation/Building instructions
