# (C) Copyright 2021- ECMWF.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
#
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation nor
# does it submit to any jurisdiction.


cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

find_package(ecbuild 3.4 REQUIRED HINTS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../ecbuild)

project(atlas-fesom LANGUAGES CXX)

find_package(eckit 1.16.1 REQUIRED)
find_package(atlas 0.45.0 REQUIRED)

if( NOT eckit_HAVE_LZ4 )
    ecbuild_warn( "eckit is required to have LZ4 support to decompress atlas-fesom binary data files. "
                  "Continuing regardless with reduced tests" )
endif()

ecbuild_add_option( FEATURE RETRIEVE_FESOM_DATA
                    DEFAULT OFF
                    DESCRIPTION "Retrieve all available atlas-fesom binary data during build step." )

# The above is equivalent to calling as part of build step:
#
#    ATLAS_FESOM_CACHING=1 ATLAS_CACHE_PATH=<build-dir>/share <build-dir>/bin/atlas-fesom-cache --grid=all
#
# To be more selective in which grids to retrieve, don't enable above option but instead run after building, for each grid:
#
#    ATLAS_FESOM_CACHING=1 ATLAS_CACHE_PATH=<build-dir>/share <build-dir>/bin/atlas-fesom-cache --grid=<gridname>
#
# Or just for installation, after installation run for each grid:
#
#    ATLAS_FESOM_CACHING=1 ATLAS_CACHE_PATH=<install-dir>/share <install-dir>/bin/atlas-fesom-cache --grid=<gridname>
#

ecbuild_add_option( FEATURE INSTALL_FESOM_DATA
                    DEFAULT OFF
                    DESCRIPTION "Install available atlas-fesom binary data present in build dir" )

atlas_create_plugin( atlas-fesom
    URL       https://sites.ecmwf.int/docs/atlas
    NAMESPACE int.ecmwf )


if( CMAKE_CXX_COMPILER_ID MATCHES NVHPC )
    ecbuild_add_cxx_flags("--diag_suppress declared_but_not_referenced --display_error_number" NAME atlas_fesom_cxx_disable_warnings )
    # For all the variables with side effects (constructor/dectructor functionality)
  endif()

add_subdirectory(etc)
add_subdirectory(share)
add_subdirectory(src)

ecbuild_install_project(NAME ${PROJECT_NAME})

