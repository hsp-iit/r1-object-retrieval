################################################################################
#                                                                              #
# Copyright (C) 2020 Fondazione Istituto Italiano di Tecnologia (IIT)          #
# All Rights Reserved.                                                         #
#                                                                              #
################################################################################

set(appname raise_llm)

file(GLOB apps      ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.xml)
file(GLOB conf      ${CMAKE_CURRENT_SOURCE_DIR}/conf/*.ini)
file(GLOB prompt      ${CMAKE_CURRENT_SOURCE_DIR}/conf/*_prompt.txt)
file(GLOB robotInterface      ${CMAKE_CURRENT_SOURCE_DIR}/conf/robotInterface/*.xml)

yarp_install(FILES ${apps}    DESTINATION ${${PROJECT_NAME}_APPLICATIONS_INSTALL_DIR})
yarp_install(FILES ${conf}    DESTINATION ${${PROJECT_NAME}_CONTEXTS_INSTALL_DIR}/${appname})
yarp_install(FILES ${prompt}    DESTINATION ${${PROJECT_NAME}_CONTEXTS_INSTALL_DIR}/${appname})
yarp_install(FILES ${robotInterface}    DESTINATION ${${PROJECT_NAME}_CONTEXTS_INSTALL_DIR}/${appname}/robotInterface)
