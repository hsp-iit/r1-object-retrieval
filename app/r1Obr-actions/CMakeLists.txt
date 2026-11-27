set(appname r1Obr-actions)

file(GLOB conf      ${CMAKE_CURRENT_SOURCE_DIR}/conf/*.ini)
file(GLOB actions   ${CMAKE_CURRENT_SOURCE_DIR}/conf/actions/*.txt)
file(GLOB keyframes ${CMAKE_CURRENT_SOURCE_DIR}/conf/keyframes/*.txt)
file(GLOB apps      ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.xml)

yarp_install(FILES ${conf}      DESTINATION ${${PROJECT_NAME}_CONTEXTS_INSTALL_DIR}/${appname})
yarp_install(FILES ${actions}   DESTINATION ${${PROJECT_NAME}_CONTEXTS_INSTALL_DIR}/${appname}/actions)
yarp_install(FILES ${keyframes} DESTINATION ${${PROJECT_NAME}_CONTEXTS_INSTALL_DIR}/${appname}/keyframes)
yarp_install(FILES ${apps}      DESTINATION ${${PROJECT_NAME}_APPLICATIONS_INSTALL_DIR})
