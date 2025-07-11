# Default values
ros_distro=$ROS_DISTRO
r1ObrRemote=$R1_OBR_REMOTE
r1ObrBranch=$R1_OBR_BRANCH
tourRemote=$TOUR_REMOTE
tourBranch=$TOUR_BRANCH


. /opt/ros/${ros_distro}/setup.sh
. ${ROBOT_CODE}/yarp-devices-ros2/ros2_interfaces_ws/install/setup.sh
#. ${ROBOT_CODE}/tour-guide-robot/app/navigation2/scripts/evaluate_ip.sh

yarp conf 192.168.100.10 10000

cd ${ROBOT_CODE}/tour-guide-robot/ && rm -rf build && mkdir build && \
    (git fetch ${tourRemote} || (git remote add ${tourRemote} https://github.com/${tourRemote}/tour-guide-robot && git fetch ${tourRemote})) && git checkout ${tourBranch} && git pull && cd build && cmake .. && make -j11 && \
    cd ../aux_modules/ros2_packages && colcon build && echo ". ${ROBOT_CODE}/tour-guide-robot/aux_modules/ros2_packages/install/setup.bash" >> /home/user1/.bashrc

cd ${ROBOT_CODE}/r1-object-retrieval/ && rm -rf build && mkdir build && \
    (git fetch ${r1ObrRemote} || (git remote add ${r1ObrRemote} https://github.com/${r1ObrRemote}/r1-object-retrieval && git fetch ${r1ObrRemote})) && git checkout ${r1ObrBranch} && git pull && cd build && cmake .. && make -j11

exec "$@"
