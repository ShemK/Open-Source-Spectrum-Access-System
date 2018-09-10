# Copyright 2011,2012,2016 Free Software Foundation, Inc.
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
# Setup library
########################################################################
include(GrPlatform) #define LIB_SUFFIX

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})
list(APPEND utils_sources
    log10_vfvf_impl.cc
    psd_cvf_impl.cc
    pipe_sink_impl.cc
    shmem_write_impl.cc
    shmem_read_impl.cc
    reader_impl.cc
    gps_reader_impl.cc
    fbpsd_cvf_impl.cc
    cstates_read_impl.cc
    liquid_buffer_impl.cc
    multichan_ed_impl.cc )

set(utils_sources "${utils_sources}" PARENT_SCOPE)
if(NOT utils_sources)
	MESSAGE(STATUS "No C++ sources... skipping lib/")
	return()
endif(NOT utils_sources)

add_library(gnuradio-utils SHARED ${utils_sources})
target_link_libraries(gnuradio-utils ${Boost_LIBRARIES} ${GNURADIO_ALL_LIBRARIES} ${GNURADIO_RUNTIME_LIBRARIES})
set_target_properties(gnuradio-utils PROPERTIES DEFINE_SYMBOL "gnuradio_utils_EXPORTS")

if(APPLE)
    set_target_properties(gnuradio-utils PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif(APPLE)

########################################################################
# Install built library files
########################################################################
include(GrMiscUtils)
GR_LIBRARY_FOO(gnuradio-utils RUNTIME_COMPONENT "utils_runtime" DEVEL_COMPONENT "utils_devel")

########################################################################
# Build and register unit test
########################################################################
include(GrTest)

include_directories(${CPPUNIT_INCLUDE_DIRS})

list(APPEND test_utils_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/test_utils.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_utils.cc
)

add_executable(test-utils ${test_utils_sources})

target_link_libraries(
  test-utils
  ${GNURADIO_RUNTIME_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CPPUNIT_LIBRARIES}
  gnuradio-utils
)

GR_ADD_TEST(test_utils test-utils)

########################################################################
# Print summary
########################################################################
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Building for version: ${VERSION} / ${LIBVER}")

