# Copyright 2011,2012,2014,2016 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.6)
project(gr-sas CXX C)
enable_testing()

#install to PyBOMBS target prefix if defined
if(DEFINED ENV{PYBOMBS_PREFIX})
    set(CMAKE_INSTALL_PREFIX $ENV{PYBOMBS_PREFIX})
    message(STATUS "PyBOMBS installed GNU Radio. Setting CMAKE_INSTALL_PREFIX to $ENV{PYBOMBS_PREFIX}")
endif()

#select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

#make sure our local CMake Modules path comes first
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules)

# Set the version information here
set(VERSION_INFO_MAJOR_VERSION 1)
set(VERSION_INFO_API_COMPAT    0)
set(VERSION_INFO_MINOR_VERSION 0)
set(VERSION_INFO_MAINT_VERSION git)

# Set cmake policies.
# This will suppress developer warnings during the cmake process that can occur
# if a newer cmake version than the minimum is used.

if(POLICY CMP0026)
    cmake_policy(SET CMP0026 OLD)
endif()
if(POLICY CMP0043)
    cmake_policy(SET CMP0043 OLD)
endif()
if(POLICY CMP0045)
    cmake_policy(SET CMP0045 OLD)
endif()
if(POLICY CMP0046)
    cmake_policy(SET CMP0046 OLD)
endif()

########################################################################
# Compiler specific setup
########################################################################
if(CMAKE_COMPILER_IS_GNUCXX AND NOT WIN32)
    #http://gcc.gnu.org/wiki/Visibility
    add_definitions(-fvisibility=hidden)
endif()

SET(GCC_COVERAGE_LINK_FLAGS   "-std=c++11 -I/usr/include -L/usr/local/lib -lpxx -lpq")

add_definitions(${GCC_COVERAGE_LINK_FLAGS})

########################################################################
# Find boost
########################################################################
if(UNIX AND EXISTS "/usr/lib64")
    list(APPEND BOOST_LIBRARYDIR "/usr/lib64") #fedora 64-bit fix
endif(UNIX AND EXISTS "/usr/lib64")
set(Boost_ADDITIONAL_VERSIONS
    "1.35.0" "1.35" "1.36.0" "1.36" "1.37.0" "1.37" "1.38.0" "1.38" "1.39.0" "1.39"
    "1.40.0" "1.40" "1.41.0" "1.41" "1.42.0" "1.42" "1.43.0" "1.43" "1.44.0" "1.44"
    "1.45.0" "1.45" "1.46.0" "1.46" "1.47.0" "1.47" "1.48.0" "1.48" "1.49.0" "1.49"
    "1.50.0" "1.50" "1.51.0" "1.51" "1.52.0" "1.52" "1.53.0" "1.53" "1.54.0" "1.54"
    "1.55.0" "1.55" "1.56.0" "1.56" "1.57.0" "1.57" "1.58.0" "1.58" "1.59.0" "1.59"
    "1.60.0" "1.60" "1.61.0" "1.61" "1.62.0" "1.62" "1.63.0" "1.63" "1.64.0" "1.64"
    "1.65.0" "1.65" "1.66.0" "1.66" "1.67.0" "1.67" "1.68.0" "1.68" "1.69.0" "1.69"
)
find_package(Boost "1.35" COMPONENTS filesystem system)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost required to compile sas")
endif()

########################################################################
# Install directories
########################################################################
include(GrPlatform) #define LIB_SUFFIX
set(GR_RUNTIME_DIR      bin)
set(GR_LIBRARY_DIR      lib${LIB_SUFFIX})
set(GR_INCLUDE_DIR      include/sas)
set(GR_DATA_DIR         share)
set(GR_PKG_DATA_DIR     ${GR_DATA_DIR}/${CMAKE_PROJECT_NAME})
set(GR_DOC_DIR          ${GR_DATA_DIR}/doc)
set(GR_PKG_DOC_DIR      ${GR_DOC_DIR}/${CMAKE_PROJECT_NAME})
set(GR_CONF_DIR         etc)
set(GR_PKG_CONF_DIR     ${GR_CONF_DIR}/${CMAKE_PROJECT_NAME}/conf.d)
set(GR_LIBEXEC_DIR      libexec)
set(GR_PKG_LIBEXEC_DIR  ${GR_LIBEXEC_DIR}/${CMAKE_PROJECT_NAME})
set(GRC_BLOCKS_DIR      ${GR_PKG_DATA_DIR}/grc/blocks)

########################################################################
# On Apple only, set install name and use rpath correctly, if not already set
########################################################################
if(APPLE)
    if(NOT CMAKE_INSTALL_NAME_DIR)
        set(CMAKE_INSTALL_NAME_DIR
            ${CMAKE_INSTALL_PREFIX}/${GR_LIBRARY_DIR} CACHE
            PATH "Library Install Name Destination Directory" FORCE)
    endif(NOT CMAKE_INSTALL_NAME_DIR)
    if(NOT CMAKE_INSTALL_RPATH)
        set(CMAKE_INSTALL_RPATH
            ${CMAKE_INSTALL_PREFIX}/${GR_LIBRARY_DIR} CACHE
            PATH "Library Install RPath" FORCE)
    endif(NOT CMAKE_INSTALL_RPATH)
    if(NOT CMAKE_BUILD_WITH_INSTALL_RPATH)
        set(CMAKE_BUILD_WITH_INSTALL_RPATH ON CACHE
            BOOL "Do Build Using Library Install RPath" FORCE)
    endif(NOT CMAKE_BUILD_WITH_INSTALL_RPATH)
endif(APPLE)

########################################################################
# Find gnuradio build dependencies
########################################################################
find_package(CppUnit)
find_package(Doxygen)

# Search for GNU Radio and its components and versions. Add any
# components required to the list of GR_REQUIRED_COMPONENTS (in all
# caps such as FILTER or FFT) and change the version to the minimum
# API compatible version required.
set(GR_REQUIRED_COMPONENTS RUNTIME)
find_package(Gnuradio "3.7.2" REQUIRED)
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules)
include(GrVersion)

if(NOT CPPUNIT_FOUND)
    message(FATAL_ERROR "CppUnit required to compile sas")
endif()

########################################################################
# Setup doxygen option
########################################################################
if(DOXYGEN_FOUND)
	option(ENABLE_DOXYGEN "Build docs using Doxygen" ON)
else(DOXYGEN_FOUND)
	option(ENABLE_DOXYGEN "Build docs using Doxygen" OFF)
endif(DOXYGEN_FOUND)



########################################################################
# Find postgreSQL  and libpqxx




set(PostgreSQL_KNOWN_VERSIONS ${PostgreSQL_ADDITIONAL_VERSIONS}
     "9.6" "9.5" "9.4" "9.3" "9.2" "9.1" "9.0" "8.4" "8.3" "8.2" "8.1" "8.0")

# Define additional search paths for root directories.
set( PostgreSQL_ROOT_DIRECTORIES
   ENV PostgreSQL_ROOT
   ${PostgreSQL_ROOT}
)
foreach(suffix ${PostgreSQL_KNOWN_VERSIONS})
  if(WIN32)
    list(APPEND PostgreSQL_LIBRARY_ADDITIONAL_SEARCH_SUFFIXES
        "PostgreSQL/${suffix}/lib")
    list(APPEND PostgreSQL_INCLUDE_ADDITIONAL_SEARCH_SUFFIXES
        "PostgreSQL/${suffix}/include")
    list(APPEND PostgreSQL_TYPE_ADDITIONAL_SEARCH_SUFFIXES
        "PostgreSQL/${suffix}/include/server")
  endif()
  if(UNIX)
    list(APPEND PostgreSQL_LIBRARY_ADDITIONAL_SEARCH_SUFFIXES
        "pgsql-${suffix}/lib")
    list(APPEND PostgreSQL_INCLUDE_ADDITIONAL_SEARCH_SUFFIXES
        "pgsql-${suffix}/include")
    list(APPEND PostgreSQL_TYPE_ADDITIONAL_SEARCH_SUFFIXES
        "postgresql/${suffix}/server"
        "pgsql-${suffix}/include/server")
  endif()
endforeach()

#
# Look for an installation.
#
find_path(PostgreSQL_INCLUDE_DIR
  NAMES libpq-fe.h
  PATHS
   # Look in other places.
   ${PostgreSQL_ROOT_DIRECTORIES}
  PATH_SUFFIXES
    pgsql
    postgresql
    include
    ${PostgreSQL_INCLUDE_ADDITIONAL_SEARCH_SUFFIXES}
  # Help the user find it if we cannot.
  DOC "The ${PostgreSQL_INCLUDE_DIR_MESSAGE}"
)


# The PostgreSQL library.
set (PostgreSQL_LIBRARY_TO_FIND pq)
# Setting some more prefixes for the library
set (PostgreSQL_LIB_PREFIX "")
if ( WIN32 )
  set (PostgreSQL_LIB_PREFIX ${PostgreSQL_LIB_PREFIX} "lib")
  set (PostgreSQL_LIBRARY_TO_FIND ${PostgreSQL_LIB_PREFIX}${PostgreSQL_LIBRARY_TO_FIND})
endif()

find_library(PostgreSQL_LIBRARY
 NAMES ${PostgreSQL_LIBRARY_TO_FIND}
 PATHS
   ${PostgreSQL_ROOT_DIRECTORIES}
 PATH_SUFFIXES
   lib
   ${PostgreSQL_LIBRARY_ADDITIONAL_SEARCH_SUFFIXES}
 # Help the user find it if we cannot.
 DOC "The ${PostgreSQL_LIBRARY_DIR_MESSAGE}"
)
get_filename_component(PostgreSQL_LIBRARY_DIR ${PostgreSQL_LIBRARY} PATH)

if (PostgreSQL_INCLUDE_DIR)
  # Some platforms include multiple pg_config.hs for multi-lib configurations
  # This is a temporary workaround.  A better solution would be to compile
  # a dummy c file and extract the value of the symbol.
  file(GLOB _PG_CONFIG_HEADERS "${PostgreSQL_INCLUDE_DIR}/pg_config*.h")
  foreach(_PG_CONFIG_HEADER ${_PG_CONFIG_HEADERS})
    if(EXISTS "${_PG_CONFIG_HEADER}")
      file(STRINGS "${_PG_CONFIG_HEADER}" pgsql_version_str
           REGEX "^#define[\t ]+PG_VERSION[\t ]+\".*\"")
      if(pgsql_version_str)
        string(REGEX REPLACE "^#define[\t ]+PG_VERSION[\t ]+\"([^\"]*)\".*"
               "\\1" PostgreSQL_VERSION_STRING "${pgsql_version_str}")
        break()
      endif()
    endif()
  endforeach()
  unset(pgsql_version_str)
endif()

# Did we find anything?
#include(${CMAKE_CURRENT_LIST_DIR}/FindPackageHandleStandardArgs.cmake)
#find_package_handle_standard_args(PostgreSQL
#                                  REQUIRED_VARS PostgreSQL_LIBRARY PostgreSQL_INCLUDE_DIR PostgreSQL_TYPE_INCLUDE_DIR
#                                  VERSION_VAR PostgreSQL_VERSION_STRING)
#set(PostgreSQL_FOUND  ${POSTGRESQL_FOUND})

# Now try to get the include and library path.
if(PostgreSQL_FOUND)
  set(PostgreSQL_INCLUDE_DIRS ${PostgreSQL_INCLUDE_DIR} )
  set(PostgreSQL_LIBRARY_DIRS ${PostgreSQL_LIBRARY_DIR} )
  set(PostgreSQL_LIBRARIES ${PostgreSQL_LIBRARY})
endif()

mark_as_advanced(PostgreSQL_INCLUDE_DIR PostgreSQL_LIBRARY )


include (FindPackageHandleStandardArgs)

find_path (PQXX_INCLUDE_DIRS
    NAME
        pqxx
    PATHS
        /usr/include
        /usr/local/include
    PATH_SUFFIXES
        pqxx
    DOC 
        "Directory for pqxx headers"    
)

find_library (PQXX_LIBRARIES
    NAMES
        pqxx
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS("PQXX"
    "libpqxx couldn't be found"
    PQXX_LIBRARIES
    PQXX_INCLUDE_DIRS
)

mark_as_advanced (PQXX_INCLUDE_DIR PQXX_LIBRARY)
########################################################################
# Setup the include and linker paths
########################################################################
include_directories(
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/lib
    ${CMAKE_BINARY_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${CPPUNIT_INCLUDE_DIRS}
    ${GNURADIO_ALL_INCLUDE_DIRS}
    ${PostgreSQL_INCLUDE_DIRS}
    ${PQXX_INCLUDE_DIRS}
)

link_directories(
    ${Boost_LIBRARY_DIRS}
    ${CPPUNIT_LIBRARY_DIRS}
    ${GNURADIO_RUNTIME_LIBRARY_DIRS}
    ${PostgreSQL_LIBRARY_DIRS}
)

# Set component parameters
set(GR_SAS_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE INTERNAL "" FORCE)
set(GR_SAS_SWIG_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/swig CACHE INTERNAL "" FORCE)

########################################################################
# Create uninstall target
########################################################################
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
@ONLY)

add_custom_target(uninstall
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)

########################################################################
# Add subdirectories
########################################################################
add_subdirectory(include/sas)
add_subdirectory(lib)
add_subdirectory(swig)
add_subdirectory(python)
add_subdirectory(grc)
add_subdirectory(apps)
add_subdirectory(docs)

########################################################################
# Install cmake search helper for this library
########################################################################
if(NOT CMAKE_MODULES_DIR)
  set(CMAKE_MODULES_DIR lib${LIB_SUFFIX}/cmake)
endif(NOT CMAKE_MODULES_DIR)

install(FILES cmake/Modules/sasConfig.cmake
    DESTINATION ${CMAKE_MODULES_DIR}/sas
)
