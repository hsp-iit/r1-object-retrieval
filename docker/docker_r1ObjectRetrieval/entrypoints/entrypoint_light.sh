source /home/user1/.entrypoint.sh
cd ${ROBOT_CODE}/r1-object-retrieval/ && \
    git pull && cd build && cmake .. && make -j11 && \
    cd ${ROBOT_CODE}/tour-guide-robot/ && git pull && cd build && cmake .. && make -j11
exec "$@"
