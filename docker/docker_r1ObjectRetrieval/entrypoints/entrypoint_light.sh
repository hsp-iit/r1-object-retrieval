#!/bin/bash

# Default values
ros_distro=$ROS_DISTRO
r1ObrRemote=$R1_OBR_REMOTE
r1ObrBranch=$R1_OBR_BRANCH

# Usage function
usage() {
  echo "Usage: $0 [options]"
  echo "  -r, --ros_distro value   Set ros_distro (default: jazzy)"
  echo "  -r1, --r1ObrRemote value   Set r1ObrRemote (default: origin)"
  echo "  -b, --r1ObrBranch value   Set r1ObrBranch (default: master)"
  echo "  -h, --help       Display this help message"
  exit 1
}

# Parse arguments
while [[ $# -gt 0 ]]; do
  key="$1"
  case $key in
    -r|--ros_distro)
      ros_distro="$2"
      shift; shift
      ;;
    -r1|--r1ObrRemote)
      r1ObrRemote="$2"
      shift; shift
      ;;
    -b|--r1ObrBranch)
      r1ObrBranch="$2"
      shift; shift
      ;;
    -h|--help)
      usage
      ;;
    *)
      echo "Unknown option: $1"
      usage
      ;;
  esac
done

. /opt/ros/${ros_distro}/setup.sh
. ${ROBOT_CODE}/yarp-devices-ros2/ros2_interfaces_ws/install/setup.sh
. ${ROBOT_CODE}/tour-guide-robot/app/navigation2/scripts/evaluate_ip.sh
cd ${ROBOT_CODE}/tour-guide-robot/ && git pull && cd build && cmake .. && make -j11
cd ${ROBOT_CODE}/r1-object-retrieval/ && \
    (git fetch ${r1ObrRemote} || (git remote add ${r1ObrRemote} https://github.com/${r1ObrRemote}/r1-object-retrieval && git fetch ${r1ObrRemote})) && git checkout ${r1ObrBranch} && git pull && cd build && cmake .. && make -j11
exec "$@"
