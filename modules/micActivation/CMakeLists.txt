project(micActivation)

add_executable(${PROJECT_NAME} src/Module.cpp main.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} PRIVATE YARP::YARP_os YARP::YARP_dev YARP::YARP_init)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
