set(appname micActivation)

file(GLOB conf      ${CMAKE_CURRENT_SOURCE_DIR}/conf/*.ini)

yarp_install(FILES ${conf}    DESTINATION ${${PROJECT_NAME}_CONTEXTS_INSTALL_DIR}/${appname})

