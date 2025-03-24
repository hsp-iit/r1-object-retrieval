#!/bin/bash

ROS_DISTRO="iron"
IMAGE_NAME="colombraf/r1images:r1ObjectRetrievalSim"

DOCKER_BUILDKIT=0 docker build $1 --build-arg ros_distro=$ROS_DISTRO -t $IMAGE_NAME .
