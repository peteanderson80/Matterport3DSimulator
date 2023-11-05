#! /bin/bash

docker run --gpus all -it --mount type=bind,source=$MATTERPORT_DATA_DIR,target=/root/mount/Matterport3DSimulator/data/v1/scans \
--volume /home/cesare/Projects/Matterport3DSimulator/:/root/mount/Matterport3DSimulator \
--volume /home/cesare/.conda/:/root/mount/conda \
  mattersim:9.2-devel-ubuntu18.04

