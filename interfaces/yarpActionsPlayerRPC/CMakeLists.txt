################################################################################
#                                                                              #
# Copyright (C) 2026 Fondazione Istituto Italiano di Tecnologia (IIT)          #
# All Rights Reserved.                                                         #
#                                                                              #
################################################################################

# Client library for the RPC interface of yarpActionsPlayer
# (generated IDL client + ActionsPlayerClient helper)
add_library(yarpActionsPlayerRPC STATIC)
yarp_add_idl(IDL_GEN_FILES yarpActionsPlayer.thrift)
target_sources(yarpActionsPlayerRPC
  PRIVATE
    ${IDL_GEN_FILES}
    ActionsPlayerClient.cpp
    ActionsPlayerClient.h)
target_include_directories(yarpActionsPlayerRPC
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(yarpActionsPlayerRPC
  PUBLIC
    YARP::YARP_os)
