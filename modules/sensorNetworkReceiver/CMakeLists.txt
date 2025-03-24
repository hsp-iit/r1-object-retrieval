#
# Copyright (C) 2016 iCub Facility - IIT Istituto Italiano di Tecnologia
# Author: Raffaele Colombo raffaele.colombo@iit.it
# CopyPolicy: Released under the terms of the GNU GPL v2.0.
#

project(sensorNetworkReceiver)

file(GLOB folder_source *.cpp)
file(GLOB folder_header *.h)

source_group("Source Files" FILES ${folder_source})
source_group("Header Files" FILES ${folder_header})

include_directories(${ICUB_INCLUDE_DIRS})
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${folder_source} ${folder_header})
target_link_libraries(${PROJECT_NAME} PRIVATE ${YARP_LIBRARIES} r1OrchestratorRPC)
set_property(TARGET sensorNetworkReceiver PROPERTY FOLDER "Modules")
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
