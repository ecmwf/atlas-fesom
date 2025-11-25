# (C) Copyright 2024- ECMWF.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.

# - Try to find METIS
# Once done this will define
#
#  METIS_FOUND         - system has METIS
#  METIS_INCLUDE_DIRS  - the METIS include directories
#  METIS_LIBRARIES     - link these to use METIS
#  METIS_VERSION.      - verion of METIS
#
# The following paths will be searched with priority if set in CMake or env
#
#  METIS_ROOT          - root directory of the METIS installation
#  METIS_PATH          - root directory of the METIS installation

# Search with priority for METIS_ROOT, METIS_PATH
find_path(METIS_INCLUDE_DIR metis.h
          PATHS ${METIS_ROOT} ${METIS_PATH} $ENV{METIS_ROOT} $ENV{METIS_PATH}
          PATH_SUFFIXES include NO_DEFAULT_PATH)

find_path(METIS_INCLUDE_DIR metis.h
          PATH_SUFFIXES include)

if( METIS_INCLUDE_DIR ) # use include dir to find libs

    set( METIS_INCLUDE_DIRS ${METIS_INCLUDE_DIR} )

    find_library( METIS_LIB
                  PATHS ${METIS_ROOT} ${METIS_PATH} $ENV{METIS_ROOT} $ENV{METIS_PATH}
                  PATH_SUFFIXES lib
                  NAMES metis )

    if( METIS_LIB )
        set( METIS_LIBRARIES ${METIS_LIB} )
    endif()

    file (READ ${METIS_INCLUDE_DIR}/metis.h _METIS_VERSION_CONTENTS)

    string (REGEX REPLACE ".*#define METIS_VER_MAJOR[ \t]+([0-9]+).*" "\\1"
      METIS_VERSION_MAJOR "${_METIS_VERSION_CONTENTS}")
    string (REGEX REPLACE ".*#define METIS_VER_MINOR[ \t]+([0-9]+).*" "\\1"
      METIS_VERSION_MINOR "${_METIS_VERSION_CONTENTS}")
    string (REGEX REPLACE ".*#define METIS_VER_SUBMINOR[ \t]+([0-9]+).*" "\\1"
      METIS_VERSION_PATCH "${_METIS_VERSION_CONTENTS}")

    set (METIS_VERSION
      ${METIS_VERSION_MAJOR}.${METIS_VERSION_MINOR}.${METIS_VERSION_PATCH})

endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args (METIS REQUIRED_VARS
  METIS_INCLUDE_DIRS METIS_LIBRARIES VERSION_VAR METIS_VERSION)

mark_as_advanced( METIS_INCLUDE_DIR METIS_LIB )
