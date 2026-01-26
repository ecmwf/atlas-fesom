# (C) Copyright 2021- ECMWF.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
#
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation nor
# does it submit to any jurisdiction.

ecbuild_add_option( FEATURE WARNING_AS_ERROR
                    DEFAULT OFF
                    DESCRIPTION "Treat compile warning as error" )

if(HAVE_WARNING_AS_ERROR)
  ecbuild_add_cxx_flags("-Werror" NO_FAIL NAME atlas_fesom_cxx_warning_as_error)
endif()

ecbuild_add_option( FEATURE WARNINGS
                    DEFAULT ON
                    DESCRIPTION "Add warnings to compiler" )

# activate warnings, ecbuild macros check the compiler recognises the options
if(HAVE_WARNINGS)

  ecbuild_add_cxx_flags("-Wall" NO_FAIL)
  ecbuild_add_cxx_flags("-Wextra" NO_FAIL)

  ecbuild_add_cxx_flags("-Wno-unused-parameter" NO_FAIL)
#   ecbuild_add_cxx_flags("-Wno-sign-compare" NO_FAIL)

endif()

add_subdirectory(atlas-fesom)
add_subdirectory(tests)
add_subdirectory(apps)
