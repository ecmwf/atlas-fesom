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

#include "atlas/array.h"
#include "atlas/field/Field.h"
#include "atlas/grid/Distribution.h"
#include "atlas/grid/Partitioner.h"
#include "atlas/grid/UnstructuredGrid.h"
#include "atlas/library/config.h"
#include "atlas/mesh.h"
#include "atlas/meshgenerator/detail/MeshGeneratorFactory.h"
#include "atlas/parallel/mpi/mpi.h"
#include "atlas/runtime/Exception.h"
#include "atlas/runtime/Log.h"
#include "atlas/util/CoordinateEnums.h"
#include "atlas/util/Geometry.h"
#include "atlas-fesom/util/AtlasIOReader.h"

namespace atlas {
namespace fesom {
namespace meshgenerator {

FesomMeshGenerator::FesomMeshGenerator( const eckit::Parametrisation& p) {
    mpi_comm_ = mpi::comm().name();
    p.get("mpi_comm", mpi_comm_);
    auto& comm = mpi::comm(mpi_comm_);
    nb_parts_ = comm.size();
    part_ = comm.rank();
    p.get("nb_parts", nb_parts_);
    p.get("part", part_);
}


void FesomMeshGenerator::generate( const Grid& grid, const grid::Distribution& distribution, Mesh& mesh ) const {
    ATLAS_TRACE( "FesomMeshGenerator::generate" );
    auto& comm = mpi::comm(mpi_comm_);

    atlas::vector<int64_t> glb_cell2node_v;
    int64_t glb_nb_cells;

    ATLAS_TRACE_SCOPE("Read + broadcast global connectivity") {
        fesom::AtlasIOReader read(grid.spec().getString("data"), mpi_comm_);

        if (comm.rank() == 0) {
            glb_nb_cells = read.nb_cells();
        }
        comm.broadcast(glb_nb_cells,0);
        glb_cell2node_v.resize(glb_nb_cells*3);
        if (comm.rank() == 0) {
            read.reader_.read("connectivity_cell2node", glb_cell2node_v).wait();
        }
        comm.broadcast(glb_cell2node_v.data(), glb_cell2node_v.size() , 0);
    }
    auto glb_cell2node = array::make_view<const int64_t,2>(glb_cell2node_v.data(),array::make_shape(glb_nb_cells, 3));

    ATLAS_TRACE_SCOPE("Create mesh partition") {
        atlas::vector<idx_t> partition_cells(glb_nb_cells,0);
        atlas::vector<idx_t> partition_nodes(grid.size(),0);
        atlas_omp_parallel_for (idx_t jcell=0; jcell < glb_nb_cells; ++jcell) {
            for (idx_t jnode=0; jnode < 3; ++jnode ) {
                auto node = glb_cell2node(jcell,jnode);
                if (distribution.partition(node) == part_) {
                    partition_cells[jcell] = 1; // Mark this cell as candidate
                }
            }
        }
        // It is still possible that a cell is marked as candidate on multiple ranks. We need to disambiguate
        // The element gets assigned to the partition of which most element nodes belong
        // In case each node belongs to a different partition, assign the element to the partition of the first node
        atlas_omp_parallel_for (idx_t jcell=0; jcell < glb_nb_cells; ++jcell) {
            if (not partition_cells[jcell]) {
                continue;
            }
            auto node0 = glb_cell2node(jcell,0);
            auto node1 = glb_cell2node(jcell,1);
            auto node2 = glb_cell2node(jcell,2);
            int p0 = distribution.partition(node0);
            int p1 = distribution.partition(node1);
            int p2 = distribution.partition(node2);

            if (p0 != p1 && p1 != p2 && p2 != p0) {
                // Arbitrarily choose p0
                partition_cells[jcell] = (p0 == part_);
            }
            else {
                if (p0 == p1 || p0 == p2) {
                    partition_cells[jcell] = (p0 == part_);
                }
                else if (p1 == p2) {
                    partition_cells[jcell] = (p1 == part_);
                }
                else {
                    ATLAS_THROW_EXCEPTION("Should not be here");
                }
            }
            if (p0 == part_ || partition_cells[jcell]) {
                partition_nodes[node0] = 1;
            }
            if (p1 == part_ || partition_cells[jcell]) {
                partition_nodes[node1] = 1;
            }
            if (p2 == part_ || partition_cells[jcell]) {
                partition_nodes[node2] = 1;
            }
        }

        idx_t nb_cells = std::accumulate(partition_cells.begin(),partition_cells.end(),0);
        idx_t nb_nodes = std::accumulate(partition_nodes.begin(),partition_nodes.end(),0);

        atlas::vector<idx_t> to_local_node_numbering(grid.size());

        mesh.nodes().resize(nb_nodes);
        auto xy        = array::make_view<double, 2>(mesh.nodes().xy());
        auto lonlat    = array::make_view<double, 2>(mesh.nodes().lonlat());
        auto ghost     = array::make_view<int, 1>(mesh.nodes().ghost());
        auto gidx      = array::make_view<gidx_t, 1>(mesh.nodes().global_index());
        auto ridx      = array::make_indexview<idx_t, 1>(mesh.nodes().remote_index());
        auto partition = array::make_view<int, 1>(mesh.nodes().partition());
        auto halo      = array::make_view<int, 1>(mesh.nodes().halo());

        const auto unstructured = UnstructuredGrid(grid);
        const size_t glb_nb_nodes = static_cast<size_t>(grid.size());
        for (size_t iglb = 0, i = 0; iglb < glb_nb_nodes; ++iglb) {
            if (partition_nodes[iglb]) {
                PointLonLat p = unstructured.lonlat(iglb);
                xy(i, size_t(XX)) = p.lon();
                xy(i, size_t(YY)) = p.lat();
                // Identity projection, therefore (lon,lat) = (x,y)
                lonlat(i, size_t(LON)) = p.lon();
                lonlat(i, size_t(LAT)) = p.lat();
                ghost(i)               = 0;
                halo(i)                = 0;
                gidx(i)                = iglb+1;
                ridx(i)                = i;
                partition(i)           = part_;
                to_local_node_numbering[iglb] = i;
                ++i;
            }
            else {
                to_local_node_numbering[iglb] = -1;
            }
        }

        mesh.cells().add(mesh::ElementType::create("Triangle"), nb_cells);
        atlas::mesh::HybridElements::Connectivity& node_connectivity = mesh.cells().node_connectivity();
        auto cells_part = array::make_view<int, 1>(mesh.cells().partition());
        auto cells_gidx = array::make_view<gidx_t, 1>(mesh.cells().global_index());

        auto& triangles = node_connectivity.block(0);
        idx_t triangle[3];
        for( int64_t iglb = 0, i=0; iglb < glb_nb_cells; ++iglb) {
            if (partition_cells[iglb]) {
                cells_gidx(i) = iglb+1;
                triangle[0] = to_local_node_numbering[glb_cell2node(iglb,0)];
                triangle[1] = to_local_node_numbering[glb_cell2node(iglb,1)];
                triangle[2] = to_local_node_numbering[glb_cell2node(iglb,2)];
                triangles.set(i,triangle);
                ++i;
            }
        }
        cells_part.assign(part_);
    }

    if (grid.spec().has("edge_length_in_degrees_min") && grid.spec().has("edge_length_in_degrees_max")) {
        const double s_min = grid.spec().getDouble("edge_length_in_degrees_min");
        const double s_max = grid.spec().getDouble("edge_length_in_degrees_max");
        PointLonLat eq0_ll {0,0};
        PointLonLat eq_min_ll {s_min,0};
        PointLonLat eq_max_ll {s_max,0};
        atlas::Geometry geometry("UnitSphere");
        PointXYZ eq0 = geometry.xyz(eq0_ll);
        PointXYZ eq_min = geometry.xyz(eq_min_ll);
        PointXYZ eq_max = geometry.xyz(eq_max_ll);
        const double d_min = PointXYZ::distance(eq0, eq_min);
        const double d_max = PointXYZ::distance(eq0, eq_max);
        mesh.metadata().set("cell_minimum_diagonal_on_unit_sphere",d_min);
        mesh.metadata().set("cell_maximum_diagonal_on_unit_sphere",d_max);
        // ATLAS_DEBUG("from file");
        // Log::info() << std::setprecision(12);
        // ATLAS_DEBUG_VAR(d_min);
        // ATLAS_DEBUG_VAR(d_max);
    }
    else {
        ATLAS_TRACE_SCOPE("Compute cell_{maximum,mininum}_diagonal_on_unit_sphere") {
            // ATLAS_DEBUG("Computing min max");
            // Instead of computing, this could be part of the grid spec
            atlas::Geometry geometry("UnitSphere");
            double d2_max{0};
            double d2_min{geometry.radius()};

#define COMPUTE_EDGE_LENTH 1
// COMPUTE_EDGE_LENTH definition allows to compute the edge_length_in_degrees_{min,max} which can be added to the grid spec in the yaml file
// Really this code should belong in a standalone executable that computes it
#if COMPUTE_EDGE_LENTH
            double s_max{0};
            double s_min{2.*M_PI};
#endif
            auto lonlat    = array::make_view<double, 2>(mesh.nodes().lonlat());
            atlas::mesh::HybridElements::Connectivity& node_connectivity = mesh.cells().node_connectivity();
            auto& triangles = node_connectivity.block(0);
            for( idx_t i=0; i<mesh.cells().size(); ++i) {
                auto p0_ll = PointLonLat{ lonlat(triangles(i,0),LON), lonlat(triangles(i,0),LAT) };
                auto p1_ll = PointLonLat{ lonlat(triangles(i,1),LON), lonlat(triangles(i,1),LAT) };
                auto p2_ll = PointLonLat{ lonlat(triangles(i,2),LON), lonlat(triangles(i,2),LAT) };
#if COMPUTE_EDGE_LENTH
                auto update_s_min_max = [&](const PointLonLat& x, const PointLonLat& y) {
                    constexpr double deg_to_rad = M_PI / 180.;
                    double lon1 = x.lon() * deg_to_rad;
                    double lat1 = x.lat() * deg_to_rad;
                    double lon2 = y.lon() * deg_to_rad;
                    double lat2 = y.lat() * deg_to_rad;
                    double dlon = lon2 - lon1;
                    double dlat = lat2 - lat1;
                    auto sqr = [](double v) { return v*v; };
                    double a = sqr(std::sin(dlat*0.5)) + std::cos(lat1) * std::cos(lat2) * sqr(std::sin(dlon*0.5));
                    double s = 2. * std::atan2(std::sqrt(a), std::sqrt(1.-a));
                    s_max = std::max(s_max, s);
                    s_min = std::min(s_min, s);
                };
                update_s_min_max(p0_ll, p1_ll);
                update_s_min_max(p0_ll, p2_ll);
                update_s_min_max(p1_ll, p2_ll);
#endif
                PointXYZ p0 = geometry.xyz( p0_ll );
                PointXYZ p1 = geometry.xyz( p1_ll );
                PointXYZ p2 = geometry.xyz( p2_ll );
                auto update_d2_min_max = [&](const PointXYZ& x, const PointXYZ& y) {
                    double d2 = PointXYZ::distance2(x,y);
                    d2_max = std::max(d2_max, d2);
                    d2_min = std::min(d2_min, d2);
                };
                update_d2_min_max(p0, p1);
                update_d2_min_max(p0, p2);
                update_d2_min_max(p1, p2);
            }
            double d_min = std::sqrt(d2_min);
            double d_max = std::sqrt(d2_max);
            comm.allReduceInPlace(d_min,eckit::mpi::min());
            comm.allReduceInPlace(d_max,eckit::mpi::max());
            mesh.metadata().set("cell_minimum_diagonal_on_unit_sphere",d_min);
            mesh.metadata().set("cell_maximum_diagonal_on_unit_sphere",d_max);
#if COMPUTE_EDGE_LENTH
            Log::info() << std::setprecision(15);
            ATLAS_DEBUG_VAR(d_min);
            ATLAS_DEBUG_VAR(d_max);
            s_min *= 180./M_PI; // to degrees
            s_max *= 180./M_PI; // to degrees
            ATLAS_DEBUG_VAR(s_min);
            ATLAS_DEBUG_VAR(s_max);
            PointLonLat eq0_ll {0,0};
            PointLonLat eq_min_ll {s_min,0};
            PointLonLat eq_max_ll {s_max,0};
            PointXYZ eq0 = geometry.xyz(eq0_ll);
            PointXYZ eq_min = geometry.xyz(eq_min_ll);
            PointXYZ eq_max = geometry.xyz(eq_max_ll);
            ATLAS_DEBUG_VAR(PointXYZ::distance(eq0, eq_min));
            ATLAS_DEBUG_VAR(PointXYZ::distance(eq0, eq_max));
#endif
        }
    }
}

void FesomMeshGenerator::generate( const Grid& grid, Mesh& mesh ) const {
    mpi::push(mpi_comm_);
    grid::Partitioner partitioner( grid.partitioner().getString("type"), nb_parts_ );
    grid::Distribution distribution( partitioner.partition(grid) );
    mpi::pop();
    generate( grid, distribution, mesh );
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
