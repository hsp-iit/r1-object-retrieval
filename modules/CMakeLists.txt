# Copyright (C) 2015 iCub Facility - Istituto Italiano di Tecnologia
# Author: Raffaele Colombo
# email:  raffaele.colombo@iit.it

add_subdirectory(nextLocPlanner)
add_subdirectory(lookForObject)
add_subdirectory(goAndFindIt)
add_subdirectory(r1Obr-orchestrator)
add_subdirectory(disappointmentPose)
add_subdirectory(approachObject)
add_subdirectory(look_and_point)
add_subdirectory(micActivation)
add_subdirectory(sensorNetworkReceiver)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libfvad IMPORTED_TARGET libfvad)
if(${libfvad_FOUND})
    add_subdirectory(backupVAD)
endif()
