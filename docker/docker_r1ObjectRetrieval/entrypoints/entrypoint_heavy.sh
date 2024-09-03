cd ${ROBOT_CODE}/ycm && git pull && cd build && cmake .. && make -j11 && \
    cd ${ROBOT_CODE}/yarp && git pull && cd build && cmake .. && make -j11 && \
    cd ${ROBOT_CODE}/icub-main && git pull && cd build && cmake .. && make -j11 && \
    cd ${ROBOT_CODE}/navigation && git pull && cd build && cmake .. && make -j11 && \
    cd ${ROBOT_CODE}/robots-configuration && git pull && \
    cd ${ROBOT_CODE}/cer/ && \ git pull && cd build && cmake .. && make -j11 && \
    cd ${ROBOT_CODE}/tour-guide-robot/ && \ git pull && cd build && cmake .. && make -j11 && \
    cd ${ROBOT_CODE}/r1-object-retrieval/ && git pull && cd build && cmake .. && make -j11
