/*
 * (C) Copyright 2021- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "FesomMeshGenerator.h"

#include <algorithm>
#include <numeric>
#include <tuple>
#include <utility>

#include "eckit/utils/Hash.h"

#include "atlas/array/Array.h"
#include "atlas/array/ArrayView.h"
#include "atlas/array/IndexView.h"
#include "atlas/array/MakeView.h"
#include "atlas/field/Field.h"
#include "atlas/grid/Distribution.h"
#include "atlas/grid/Partitioner.h"
#include "atlas/grid/Spacing.h"
#include "atlas/grid/UnstructuredGrid.h"
#include "atlas/library/config.h"
#include "atlas/mesh/ElementType.h"
#include "atlas/mesh/Elements.h"
#include "atlas/mesh/HybridElements.h"
#include "atlas/mesh/Mesh.h"
#include "atlas/mesh/Nodes.h"
#include "atlas/meshgenerator/detail/MeshGeneratorFactory.h"
#include "atlas/meshgenerator/detail/StructuredMeshGenerator.h"
#include "atlas/parallel/mpi/mpi.h"
#include "atlas/runtime/Exception.h"
#include "atlas/runtime/Log.h"
#include "atlas/util/Constants.h"
#include "atlas/util/CoordinateEnums.h"
#include "atlas/util/Geometry.h"
#include "atlas/util/NormaliseLongitude.h"
#include "atlas/util/Topology.h"
#include "atlas/grid/SpecRegistry.h"
#include "atlas-fesom/util/FesomDataFile.h"
#include "atlas-fesom/util/AtlasIOReader.h"

namespace atlas {
namespace fesom {
namespace meshgenerator {

void FesomMeshGenerator::generate( const Grid& grid, const grid::Distribution& distribution, Mesh& mesh ) const {
    ATLAS_TRACE( "FesomMeshGenerator::generate" );
    using Topology = util::Topology;

    auto spec = grid::SpecRegistry::get( grid.name() );

    fesom::AtlasIOReader read(spec.getString( "data" ));
    auto nb_cells = read.nb_cells();
    std::vector<std::array<int64_t,3>> connectivity_cell2node;
    read.connectivity_cell2node(connectivity_cell2node);
    ATLAS_ASSERT(connectivity_cell2node.size() == nb_cells);

    mesh.nodes().resize(grid.size());
    auto xy        = array::make_view<double, 2>(mesh.nodes().xy());
    auto lonlat    = array::make_view<double, 2>(mesh.nodes().lonlat());
    auto ghost     = array::make_view<int, 1>(mesh.nodes().ghost());
    auto gidx      = array::make_view<gidx_t, 1>(mesh.nodes().global_index());
    auto ridx      = array::make_indexview<idx_t, 1>(mesh.nodes().remote_index());
    auto partition = array::make_view<int, 1>(mesh.nodes().partition());
    auto halo      = array::make_view<int, 1>(mesh.nodes().halo());

    auto unstructured = UnstructuredGrid(grid);
    for (size_t i = 0; i < mesh.nodes().size(); ++i) {
        PointLonLat p = unstructured.lonlat(i);
        xy(i, size_t(XX)) = p.lon();
        xy(i, size_t(YY)) = p.lat();
        // Identity projection, therefore (lon,lat) = (x,y)
        lonlat(i, size_t(LON)) = p.lon();
        lonlat(i, size_t(LAT)) = p.lat();
        ghost(i)               = 0;
        gidx(i)                = i+1;
        ridx(i)                = i;
        partition(i)           = 0;
    }
    halo.assign(0);

    mesh.cells().add(mesh::ElementType::create("Triangle"), nb_cells);
    atlas::mesh::HybridElements::Connectivity& node_connectivity = mesh.cells().node_connectivity();
    auto cells_part = array::make_view<int, 1>(mesh.cells().partition());
    auto cells_gidx = array::make_view<gidx_t, 1>(mesh.cells().global_index());
    auto cells_flags = array::make_view<int, 1>(mesh.cells().flags());

    auto& triangles = node_connectivity.block(0);
    idx_t triangle[3];
    for( size_t i = 0; i < nb_cells; ++i) {
        cells_gidx(i) = i+1;
        triangle[0] = connectivity_cell2node[i][0];
        triangle[1] = connectivity_cell2node[i][1];
        triangle[2] = connectivity_cell2node[i][2];
        triangles.set(i,triangle);
    }
    cells_part.assign(0);


    // Instead of computing, this could be part of the grid spec
    atlas::Geometry geometry("UnitSphere");
    double d2_max{0};
    double d2_min{geometry.radius()};
    for( size_t i=0; i<mesh.cells().size(); ++i) {
        auto p0_ll = PointLonLat{ lonlat(triangles(i,0),LON), lonlat(triangles(i,0),LAT) };
        auto p1_ll = PointLonLat{ lonlat(triangles(i,1),LON), lonlat(triangles(i,1),LAT) };
        auto p2_ll = PointLonLat{ lonlat(triangles(i,2),LON), lonlat(triangles(i,2),LAT) };
        PointXYZ p0 = geometry.xyz( p0_ll );
        PointXYZ p1 = geometry.xyz( p1_ll );
        PointXYZ p2 = geometry.xyz( p2_ll );
        double d2;
        d2 = PointXYZ::distance2(p0,p1);
        d2_max = std::max(d2_max, d2);
        d2_min = std::max(d2_min, d2);
        d2 = PointXYZ::distance2(p0,p2);
        d2_max = std::max(d2_max, d2);
        d2_min = std::max(d2_min, d2);
        d2 = PointXYZ::distance2(p1,p2);
        d2_max = std::max(d2_max, d2);
        d2_min = std::max(d2_min, d2);
    }
    double d_min = std::sqrt(d2_min);
    double d_max = std::sqrt(d2_max);
    mesh.metadata().set("cell_maximum_diagonal_on_unit_sphere",d_max);
    mesh.metadata().set("cell_minimum_diagonal_on_unit_sphere",d_min);
    ATLAS_DEBUG_VAR(d_max/1000.);
    ATLAS_DEBUG_VAR(d_min/1000.);
}

void FesomMeshGenerator::generate( const Grid& grid, const grid::Partitioner& partitioner, Mesh& mesh ) const {
    generate( grid, grid::Distribution( grid, partitioner ), mesh );
}

void FesomMeshGenerator::generate( const Grid& grid, Mesh& mesh ) const {
    generate( grid, grid::Partitioner( grid.partitioner() ), mesh );
}

void FesomMeshGenerator::hash( eckit::Hash& h ) const {
    h.add( "FesomMeshGenerator" );
}

namespace {
atlas::meshgenerator::MeshGeneratorBuilder<FesomMeshGenerator> __FesomMeshGenerator( "fesom" );
}

}  // namespace meshgenerator
}  // namespace fesom
}  // namespace atlas
