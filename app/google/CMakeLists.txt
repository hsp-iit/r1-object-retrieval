set(appname google)

file(GLOB conf                  ${CMAKE_CURRENT_SOURCE_DIR}/conf/*.ini)
file(GLOB robotInterface        ${CMAKE_CURRENT_SOURCE_DIR}/conf/robotInterface/*.xml)

yarp_install(FILES ${conf}              DESTINATION ${${PROJECT_NAME}_CONTEXTS_INSTALL_DIR}/${appname})
yarp_install(FILES ${robotInterface}    DESTINATION ${${PROJECT_NAME}_CONTEXTS_INSTALL_DIR}/${appname}/robotInterface)