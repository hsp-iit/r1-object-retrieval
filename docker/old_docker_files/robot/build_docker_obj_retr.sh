#!/bin/bash

ROS_DISTRO="iron"
IMAGE_NAME="colombraf/r1images:r1ObjectRetrieval"

sudo docker build $1 --build-arg ros_distro=$ROS_DISTRO -t $IMAGE_NAME .
