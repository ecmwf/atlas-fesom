/*
 * (C) Copyright 2021- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "atlas/array.h"
#include "atlas/grid.h"
#include "atlas/grid/Partitioner.h"
#include "atlas/mesh.h"
#include "atlas/meshgenerator.h"
#include "atlas/parallel/mpi/mpi.h"
#include "atlas/util/Topology.h"
#include "atlas/functionspace/NodeColumns.h"

#include "atlas/output/Gmsh.h"

#include "tests/AtlasTestEnvironment.h"

namespace atlas {
namespace test {

CASE( "test fesom mesh node ghost metadata" ) {
    Grid grid( "fesom-pi_N" );
    grid::Partitioner partitioner( "equal_regions", mpi::size() );
    Mesh mesh = MeshGenerator( "fesom" ).generate( grid, partitioner );

    auto ghost = array::make_view<int, 1>( mesh.nodes().ghost() );
    auto flags = array::make_view<int, 1>( mesh.nodes().flags() );

    idx_t ghost_count = 0;
    bool found_ghost = false;
    for ( idx_t jnode = 0; jnode < mesh.nodes().size(); ++jnode ) {
        const bool is_ghost = ghost( jnode );
        const bool has_ghost_flag = util::Topology::view( flags( jnode ) ).check( util::Topology::GHOST );
        EXPECT_EQ( has_ghost_flag, is_ghost );
        found_ghost = found_ghost || is_ghost;
        EXPECT( not found_ghost || is_ghost );
        ghost_count += is_ghost;
    }

    switch ( mpi::size() ) {
        case 1: {
            EXPECT_EQ( ghost_count, 0 );
            break;
        }
        case 2: {
            std::array<idx_t, 2> expected_ghost_count = { 36, 36};
            EXPECT_EQ( ghost_count, expected_ghost_count[mpi::rank()] );
            break;
        }
        case 3: {
            std::array<idx_t, 3> expected_ghost_count = { 20, 64, 45 };
            EXPECT_EQ( ghost_count, expected_ghost_count[mpi::rank()] );
            break;
        }
        case 4: {
            std::array<idx_t, 4> expected_ghost_count = { 25, 64, 56, 47 };
            EXPECT_EQ( ghost_count, expected_ghost_count[mpi::rank()] );
            break;
        }
        default:
            std::cout << mpi::rank() << " " << ghost_count << std::endl;
    }
    idx_t total_ghost_count = ghost_count;
    mpi::comm().allReduceInPlace( total_ghost_count, eckit::mpi::sum() );

    if (mpi::size() > 1) {
        EXPECT( total_ghost_count > 0 );
    }

    functionspace::NodeColumns fs( mesh, option::halo(1) );

    output::Gmsh gmsh( "fesom_mesh_node_ghost_metadata.msh", util::Config("ghost",true) | util::Config("coordinates", "xyz"));
    gmsh.write( mesh );

}

}  // namespace test
}  // namespace atlas

int main( int argc, char** argv ) {
    return atlas::test::run( argc, argv );
}
