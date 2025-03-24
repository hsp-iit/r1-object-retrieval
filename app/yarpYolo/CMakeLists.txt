set(appname yarpYolo)

file(GLOB conf      ${CMAKE_CURRENT_SOURCE_DIR}/conf/*.ini)
file(GLOB templates ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.template)
file(GLOB apps      ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.xml)


yarp_install(FILES ${conf}    DESTINATION ${${PROJECT_NAME}_CONTEXTS_INSTALL_DIR}/${appname})
yarp_install(FILES ${apps}    DESTINATION ${${PROJECT_NAME}_APPLICATIONS_INSTALL_DIR})
yarp_install(FILES ${templates} DESTINATION ${${PROJECT_NAME}_APPLICATIONS_TEMPLATES_INSTALL_DIR})
