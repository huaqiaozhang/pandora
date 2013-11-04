# Install script for directory: /afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/PandoraPFANew/v00-09

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/PandoraPFANew/v00-09")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "RelWithDebInfo")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

# Install shared libraries without execute permission?
IF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  SET(CMAKE_INSTALL_SO_NO_EXE "0")
ENDIF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE FILE FILES "/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/PandoraPFANew/v00-09/build/PandoraPFANewConfig.cmake")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE FILE FILES "/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/PandoraPFANew/v00-09/build/PandoraPFANewConfigVersion.cmake")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake" TYPE FILE FILES "/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/PandoraPFANew/v00-09/build/PandoraPFANewLibDeps.cmake")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  INCLUDE("/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/PandoraPFANew/v00-09/build/doc/cmake_install.cmake")
  INCLUDE("/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/PandoraPFANew/v00-09/build/Framework/cmake_install.cmake")
  INCLUDE("/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/PandoraPFANew/v00-09/build/Monitoring/cmake_install.cmake")
  INCLUDE("/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/PandoraPFANew/v00-09/build/FineGranularityContent/cmake_install.cmake")
  INCLUDE("/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/PandoraPFANew/v00-09/build/KMeansContent/cmake_install.cmake")

ENDIF(NOT CMAKE_INSTALL_LOCAL_ONLY)

IF(CMAKE_INSTALL_COMPONENT)
  SET(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
ELSE(CMAKE_INSTALL_COMPONENT)
  SET(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
ENDIF(CMAKE_INSTALL_COMPONENT)

FILE(WRITE "/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/PandoraPFANew/v00-09/build/${CMAKE_INSTALL_MANIFEST}" "")
FOREACH(file ${CMAKE_INSTALL_MANIFEST_FILES})
  FILE(APPEND "/afs/desy.de/project/ilcsoft/sw/x86_64_gcc41_sl5/v01-16/PandoraPFANew/v00-09/build/${CMAKE_INSTALL_MANIFEST}" "${file}\n")
ENDFOREACH(file)
