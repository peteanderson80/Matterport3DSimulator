# Matterport3DSimulator
# Requires nvidia gpu with driver 396.37 or higher


FROM nvidia/cudagl:11.4.2-devel-ubuntu20.04

# Install cudnn
ENV CUDNN_VERSION=8.2.4.15
LABEL com.nvidia.cudnn.version="${CUDNN_VERSION}"

RUN apt-get update && apt-get install -y --no-install-recommends \
    libcudnn8=${CUDNN_VERSION}-1+cuda11.4 \
    libcudnn8-dev=${CUDNN_VERSION}-1+cuda11.4 && \
    apt-mark hold libcudnn8 && \
    rm -rf /var/lib/apt/lists/*



# Install a few libraries to support both EGL and OSMESA options
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y wget doxygen curl libjsoncpp-dev libepoxy-dev libglm-dev libosmesa6 libosmesa6-dev libglew-dev libopencv-dev python3-opencv python3-setuptools python3-dev python3-pip
RUN pip3 install -i https://mirrors.bfsu.edu.cn/pypi/web/simple opencv-python==4.10.0.84 torch==1.12.1+cu113 torchvision==0.13.1+cu113 numpy==1.24.4 pandas==2.0.3 networkx==3.1

#install latest cmake
ADD https://cmake.org/files/v3.12/cmake-3.12.2-Linux-x86_64.sh /cmake-3.12.2-Linux-x86_64.sh
RUN mkdir /opt/cmake
RUN sh /cmake-3.12.2-Linux-x86_64.sh --prefix=/opt/cmake --skip-license
RUN ln -s /opt/cmake/bin/cmake /usr/local/bin/cmake
RUN cmake --version

ENV PYTHONPATH=/root/mount/Matterport3DSimulator/build
