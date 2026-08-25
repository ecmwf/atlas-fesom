
LIST( APPEND ATLAS_TEST_ENVIRONMENT
    ATLAS_FESOM_CACHING=1
    ATLAS_CACHE_PATH=${CMAKE_BINARY_DIR}/share )

include_directories( ${PROJECT_SOURCE_DIR}/src )

ecbuild_add_test( TARGET  atlas_test_fesom_plugin
                  SOURCES test_fesom_plugin.cc
                  LIBS    atlas
                  ENVIRONMENT ${ATLAS_TEST_ENVIRONMENT} )

ecbuild_add_test( TARGET  atlas_test_fesom_grid_specs
                  SOURCES test_fesom_grid_specs.cc
                  LIBS    atlas
                  ENVIRONMENT ${ATLAS_TEST_ENVIRONMENT} )

ecbuild_add_test( TARGET  atlas_test_fesom_grid
                  SOURCES test_fesom_grid.cc
                  LIBS    atlas-fesom
                  ENVIRONMENT ${ATLAS_TEST_ENVIRONMENT}
                  CONDITION eckit_HAVE_LZ4 )

ecbuild_add_test( TARGET  atlas_test_fesom_mesh
                  SOURCES test_fesom_mesh.cc
                  LIBS    atlas-fesom
                  MPI     4
                  ENVIRONMENT ${ATLAS_TEST_ENVIRONMENT}
                  CONDITION eckit_HAVE_LZ4 AND eckit_HAVE_MPI )

# ecbuild_add_test( TARGET  atlas_test_fesom_valid_elements
#                   SOURCES test_fesom_valid_elements.cc
#                   LIBS    atlas-fesom
#                   ENVIRONMENT ${ATLAS_TEST_ENVIRONMENT}
#                   CONDITION eckit_HAVE_LZ4 )

# ecbuild_add_test( TARGET  atlas_test_fesom_polygon_locator
#                   SOURCES test_fesom_polygon_locator.cc
#                   LIBS    atlas-fesom
#                   MPI     2
#                   ENVIRONMENT ${ATLAS_TEST_ENVIRONMENT}
#                   CONDITION eckit_HAVE_LZ4 AND eckit_HAVE_MPI)

