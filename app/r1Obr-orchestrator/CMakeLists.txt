set(appname r1Obr-orchestrator)

file(GLOB conf                  ${CMAKE_CURRENT_SOURCE_DIR}/conf/*.ini)
file(GLOB apps                  ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.xml)
file(GLOB dances                ${CMAKE_CURRENT_SOURCE_DIR}/conf/dances/*.ini)
file(GLOB templates             ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.template)

yarp_install(FILES ${conf}    DESTINATION ${${PROJECT_NAME}_CONTEXTS_INSTALL_DIR}/${appname})
yarp_install(FILES ${apps}    DESTINATION ${${PROJECT_NAME}_APPLICATIONS_INSTALL_DIR})
yarp_install(FILES ${dances}  DESTINATION ${${PROJECT_NAME}_CONTEXTS_INSTALL_DIR}/${appname}/dances)
yarp_install(FILES ${templates} DESTINATION ${${PROJECT_NAME}_APPLICATIONS_TEMPLATES_INSTALL_DIR})
