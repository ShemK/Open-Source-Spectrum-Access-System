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

include_directories(${Boost_INCLUDE_DIR}
                    ${PostgreSQL_INCLUDE_DIRS}
                    )
link_directories(${Boost_LIBRARY_DIRS}
                ${PostgreSQL_LIBRARY_DIRS}
                )
list(APPEND sas_sources
    psql_insert_impl.cc
    sas_buffer_impl.cc
    ed_threshold_impl.cc
    decision_maker.cc)

set(sas_sources "${sas_sources}" PARENT_SCOPE)
if(NOT sas_sources)
	MESSAGE(STATUS "No C++ sources... skipping lib/")
	return()
endif(NOT sas_sources)

add_library(gnuradio-sas SHARED ${sas_sources})
target_link_libraries(gnuradio-sas ${Boost_LIBRARIES} ${GNURADIO_ALL_LIBRARIES} ${PostgreSQL_LIBRARIES} ${PQXX_LIBRARIES})
set_target_properties(gnuradio-sas PROPERTIES DEFINE_SYMBOL "gnuradio_sas_EXPORTS")

if(APPLE)
    set_target_properties(gnuradio-sas PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif(APPLE)

########################################################################
# Install built library files
########################################################################
include(GrMiscUtils)
GR_LIBRARY_FOO(gnuradio-sas RUNTIME_COMPONENT "sas_runtime" DEVEL_COMPONENT "sas_devel")

########################################################################
# Build and register unit test
########################################################################
include(GrTest)

include_directories(${CPPUNIT_INCLUDE_DIRS})

list(APPEND test_sas_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sas.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_sas.cc
)

add_executable(test-sas ${test_sas_sources})

target_link_libraries(
  test-sas
  ${GNURADIO_RUNTIME_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CPPUNIT_LIBRARIES}
  gnuradio-sas
)

GR_ADD_TEST(test_sas test-sas)

########################################################################
# Print summary
########################################################################
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Building for version: ${VERSION} / ${LIBVER}")
