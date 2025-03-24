#
# Copyright (C) 2016 iCub Facility - IIT Istituto Italiano di Tecnologia
# Author: Raffaele Colombo raffaele.colombo@iit.it
# CopyPolicy: Released under the terms of the GNU GPL v2.0.
#

project(r1Obr-orchestrator)

file(GLOB folder_source *.cpp)
file(GLOB folder_header *.h)

source_group("Source Files" FILES ${folder_source})
source_group("Header Files" FILES ${folder_header})

# Search for IDL files.
file(GLOB idl_files *.thrift *.msg *.srv)
foreach(idl ${idl_files})
  yarp_idl_to_dir(${idl} ${CMAKE_BINARY_DIR}/idl IDL_SRC IDL_HDR IDL_INCLUDE)
  set(folder_source ${folder_source} ${IDL_SRC})
  set(folder_header ${folder_header} ${IDL_HDR})
  include_directories(${IDL_INCLUDE})
endforeach()

include_directories(${OpenCV_INCLUDE_DIRS} ${ICUB_INCLUDE_DIRS})
add_executable(${PROJECT_NAME} ${folder_source} ${folder_header})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBRARIES} ${YARP_LIBRARIES} r1OrchestratorRPC)
set_property(TARGET r1Obr-orchestrator PROPERTY FOLDER "Modules")
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
