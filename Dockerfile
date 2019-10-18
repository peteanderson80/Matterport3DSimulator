# Matterport3DSimulator
# Requires nvidia gpu with driver 396.37 or higher


FROM nvidia/cudagl:9.2-devel-ubuntu18.04

# Install cudnn
ENV CUDNN_VERSION 7.6.4.38
LABEL com.nvidia.cudnn.version="${CUDNN_VERSION}"

RUN apt-get update && apt-get install -y --no-install-recommends \
    libcudnn7=$CUDNN_VERSION-1+cuda9.2 \
libcudnn7-dev=$CUDNN_VERSION-1+cuda9.2 \
&& \
    apt-mark hold libcudnn7 && \
    rm -rf /var/lib/apt/lists/*


# Install a few libraries to support both EGL and OSMESA options
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y wget doxygen curl libjsoncpp-dev libepoxy-dev libglm-dev libosmesa6 libosmesa6-dev libglew-dev libopencv-dev python-opencv python3-setuptools python3-dev python3-pip
RUN pip3 install opencv-python==4.1.0.25 torch==1.1.0 torchvision==0.3.0 numpy==1.13.3 pandas==0.24.1 networkx==2.2

#install latest cmake
ADD https://cmake.org/files/v3.12/cmake-3.12.2-Linux-x86_64.sh /cmake-3.12.2-Linux-x86_64.sh
RUN mkdir /opt/cmake
RUN sh /cmake-3.12.2-Linux-x86_64.sh --prefix=/opt/cmake --skip-license
RUN ln -s /opt/cmake/bin/cmake /usr/local/bin/cmake
RUN cmake --version

ENV PYTHONPATH=/root/mount/Matterport3DSimulator/build
