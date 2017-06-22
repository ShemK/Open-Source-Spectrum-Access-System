INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_SAS sas)

FIND_PATH(
    SAS_INCLUDE_DIRS
    NAMES sas/api.h
    HINTS $ENV{SAS_DIR}/include
        ${PC_SAS_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    SAS_LIBRARIES
    NAMES gnuradio-sas
    HINTS $ENV{SAS_DIR}/lib
        ${PC_SAS_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SAS DEFAULT_MSG SAS_LIBRARIES SAS_INCLUDE_DIRS)
MARK_AS_ADVANCED(SAS_LIBRARIES SAS_INCLUDE_DIRS)

