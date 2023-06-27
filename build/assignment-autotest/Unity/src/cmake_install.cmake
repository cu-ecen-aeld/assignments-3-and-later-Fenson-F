<<<<<<< HEAD
# Install script for directory: /home/fenson/aesd-assignments/assignment-3-Fenson-F/assignments-3-and-later-Fenson-F/assignment-autotest/Unity/src
=======
# Install script for directory: /home/fenson/aesd-assignments/assignment-5-Fenson-F/assignment-5-1/assignments-3-and-later-Fenson-F/assignment-autotest/Unity/src
>>>>>>> 4d3cdb69f08954fbce6baee3e44b80a5395ce581

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
<<<<<<< HEAD
   "/home/fenson/aesd-assignments/assignment-3-Fenson-F/assignments-3-and-later-Fenson-F/assignment-autotest/Unity/src/libunity.a")
=======
   "/home/fenson/aesd-assignments/assignment-5-Fenson-F/assignment-5-1/assignments-3-and-later-Fenson-F/assignment-autotest/Unity/src/libunity.a")
>>>>>>> 4d3cdb69f08954fbce6baee3e44b80a5395ce581
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
<<<<<<< HEAD
  file(INSTALL DESTINATION "/home/fenson/aesd-assignments/assignment-3-Fenson-F/assignments-3-and-later-Fenson-F/assignment-autotest/Unity/src" TYPE STATIC_LIBRARY FILES "/home/fenson/aesd-assignments/assignment-3-Fenson-F/assignments-3-and-later-Fenson-F/build/assignment-autotest/Unity/src/libunity.a")
=======
  file(INSTALL DESTINATION "/home/fenson/aesd-assignments/assignment-5-Fenson-F/assignment-5-1/assignments-3-and-later-Fenson-F/assignment-autotest/Unity/src" TYPE STATIC_LIBRARY FILES "/home/fenson/aesd-assignments/assignment-5-Fenson-F/assignment-5-1/assignments-3-and-later-Fenson-F/build/assignment-autotest/Unity/src/libunity.a")
>>>>>>> 4d3cdb69f08954fbce6baee3e44b80a5395ce581
endif()

