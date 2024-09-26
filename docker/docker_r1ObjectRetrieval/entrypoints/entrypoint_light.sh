. /opt/ros/iron/setup.sh
. ${ROBOT_CODE}/yarp-devices-ros2/ros2_interfaces_ws/install/setup.sh
. ${ROBOT_CODE}/tour-guide-robot/app/navigation2/scripts/evaluate_ip.sh
cd ${ROBOT_CODE}/r1-object-retrieval/ && \
    git pull && cd build && cmake .. && make -j11 && \
    cd ${ROBOT_CODE}/tour-guide-robot/ && git pull && cd build && cmake .. && make -j11
exec "$@"
