INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_UTILS utils)

FIND_PATH(
    UTILS_INCLUDE_DIRS
    NAMES utils/api.h
    HINTS $ENV{UTILS_DIR}/include
        ${PC_UTILS_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    UTILS_LIBRARIES
    NAMES gnuradio-utils
    HINTS $ENV{UTILS_DIR}/lib
        ${PC_UTILS_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(UTILS DEFAULT_MSG UTILS_LIBRARIES UTILS_INCLUDE_DIRS)
MARK_AS_ADVANCED(UTILS_LIBRARIES UTILS_INCLUDE_DIRS)

