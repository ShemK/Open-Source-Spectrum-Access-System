# Install script for directory: /home/wireless/workspace/darpaSC2/rem/gr-utils/include/utils

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/wireless/gnuradio_1")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
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

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/utils" TYPE FILE FILES
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/include/utils/api.h"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/include/utils/log10_vfvf.h"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/include/utils/psd_cvf.h"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/include/utils/pipe_sink.h"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/include/utils/shmem_write.h"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/include/utils/shmem_read.h"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/include/utils/reader.h"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/include/utils/gps_reader.h"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/include/utils/fbpsd_cvf.h"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/include/utils/cstates_read.h"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/include/utils/liquid_buffer.h"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/include/utils/multichan_ed.h"
    )
endif()

