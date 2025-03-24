################################################################################
#                                                                              #
# Copyright (C) 2022 Fondazione Istituto Italiano di Tecnologia (IIT)          #
# All Rights Reserved.                                                         #
#                                                                              #
################################################################################

# Create the r1OrchestratorRPC C++ Library
add_library(r1OrchestratorRPC STATIC)
yarp_add_idl(IDL_GEN_FILES r1OrchestratorRPC.thrift)
target_sources(r1OrchestratorRPC
  PRIVATE
    ${IDL_GEN_FILES})
target_include_directories(r1OrchestratorRPC
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(r1OrchestratorRPC
  PRIVATE
    YARP::YARP_os)
