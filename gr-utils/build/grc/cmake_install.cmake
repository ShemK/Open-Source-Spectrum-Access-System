# Install script for directory: /home/wireless/workspace/darpaSC2/rem/gr-utils/grc

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/gnuradio/grc/blocks" TYPE FILE FILES
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/grc/utils_log10_vfvf.xml"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/grc/utils_psd_cvf.xml"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/grc/utils_pipe_sink.xml"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/grc/utils_shmem_write.xml"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/grc/utils_shmem_read.xml"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/grc/utils_reader.xml"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/grc/utils_streampsdvolk.xml"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/grc/utils_gps_reader.xml"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/grc/utils_ks_test.xml"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/grc/utils_anderson.xml"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/grc/utils_shapiro.xml"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/grc/utils_skewness.xml"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/grc/utils_fbpsd_cvf.xml"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/grc/utils_nmea_reader.xml"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/grc/utils_cstates_read.xml"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/grc/utils_uhd_control.xml"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/grc/utils_liquid_buffer.xml"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/grc/utils_multichan_ed.xml"
    )
endif()

