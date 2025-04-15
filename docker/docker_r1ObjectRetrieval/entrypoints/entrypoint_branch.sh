. /opt/ros/iron/setup.sh
. ${ROBOT_CODE}/yarp-devices-ros2/ros2_interfaces_ws/install/setup.sh
. ${ROBOT_CODE}/tour-guide-robot/app/navigation2/scripts/evaluate_ip.sh
cd ${ROBOT_CODE}/ycm && git pull && cd build && cmake .. && make -j11 && \
    cd ${ROBOT_CODE}/yarp && git pull && cd build && cmake .. && make -j11 && \
    cd ${ROBOT_CODE}/icub-main && git pull && cd build && cmake .. && make -j11 && \
    cd ${ROBOT_CODE}/navigation && git pull && cd build && cmake .. && make -j11 && \
    cd ${ROBOT_CODE}/robots-configuration && git pull && \
    cd ${ROBOT_CODE}/cer/ && git pull && cd build && cmake .. && make -j11 && \
    cd ${ROBOT_CODE}/yarp-devices-ros2 && git pull && cd build && cmake .. && make -j11 && \
    cd ${ROBOT_CODE}/tour-guide-robot/ && git pull && cd build && cmake .. && make -j11 && \
    cd ${ROBOT_CODE}/r1-object-retrieval/ && git remote add elandini84 https://github.com/elandini84/r1-object-retrieval && \
    git fetch elandini84 && git checkout feature/remove_dialogflow && \
    cd build && cmake .. && make -j11
exec "$@"
