# Install script for directory: /home/wireless/workspace/darpaSC2/rem/gr-utils/python

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/python2.7/dist-packages/utils" TYPE FILE FILES
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/python/__init__.py"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/python/threechannelpsd.py"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/python/streampsdvolk.py"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/python/ks_test.py"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/python/anderson.py"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/python/shapiro.py"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/python/skewness.py"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/python/nmea_reader.py"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/python/nmea_parser_core.py"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/python/uhd_control.py"
    )
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/python2.7/dist-packages/utils" TYPE FILE FILES
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/__init__.pyc"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/threechannelpsd.pyc"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/streampsdvolk.pyc"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/ks_test.pyc"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/anderson.pyc"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/shapiro.pyc"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/skewness.pyc"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/nmea_reader.pyc"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/nmea_parser_core.pyc"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/uhd_control.pyc"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/__init__.pyo"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/threechannelpsd.pyo"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/streampsdvolk.pyo"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/ks_test.pyo"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/anderson.pyo"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/shapiro.pyo"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/skewness.pyo"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/nmea_reader.pyo"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/nmea_parser_core.pyo"
    "/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python/uhd_control.pyo"
    )
endif()

