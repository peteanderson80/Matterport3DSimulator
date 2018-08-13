# Matterport3DSimulator
# Requires nvidia gpu with driver 384.xx or higher

# docker build -t mattersim .
# nvidia-docker run -it --mount type=bind,source=<MATTERPORT_DATA>,target=/root/mount/Matterport3DSimulator/data,readonly --rm mattersim bash
# ./build/tests

FROM nvidia/cudagl:9.0-devel-ubuntu16.04

# Install a few libraries
RUN apt-get update && apt-get install -y curl libjsoncpp-dev libepoxy-dev libglm-dev

# Install miniconda to /miniconda
RUN curl -LO http://repo.continuum.io/miniconda/Miniconda-latest-Linux-x86_64.sh
RUN bash Miniconda-latest-Linux-x86_64.sh -p /miniconda -b
RUN rm Miniconda-latest-Linux-x86_64.sh
ENV PATH=/miniconda/bin:${PATH}
RUN conda update -y conda

# Copy files (
ADD  . /root/mount/Matterport3DSimulator
WORKDIR /root/mount/Matterport3DSimulator

# Create a conda environment
RUN conda create --name matterport --file conda-specs.txt
ENV PATH /miniconda/envs/matterport/bin:$PATH

# Build the simulator
RUN /bin/bash -c "source activate matterport"
RUN mkdir build
WORKDIR /root/mount/Matterport3DSimulator/build
RUN cmake -DEGL_RENDERING=ON ..
RUN make
WORKDIR /root/mount/Matterport3DSimulator
