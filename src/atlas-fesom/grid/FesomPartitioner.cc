/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */


#include "FesomPartitioner.h"

#include <cstdint>
#include <vector>
#include <array>
#include <type_traits>

#if ATLAS_FESOM_HAVE_METIS
#include "metis.h"
#undef idx_t
namespace metis {
#if IDXTYPEWIDTH == 32
    using idx_t = int32_t;
#elif IDXTYPEWIDTH == 64
    using idx_t = int64_t;
#else
  #error "Incorrect user-supplied value fo IDXTYPEWIDTH"
#endif
#if REALTYPEWIDTH == 32
    using real_t = float;
#elif REALTYPEWIDTH == 64
    using real_t = double;
#else
  #error "Incorrect user-supplied value for REALTYPEWIDTH"
#endif
}
#else
namespace metis {
    using idx_t = std::int32_t;
    using real_t = double;
}
#endif

#include "pluto/pluto.h"
#include "atlas/mdspan.h"
#include "atlas/grid/Partitioner.h"
#include "atlas/grid/SpecRegistry.h"
#include "atlas/grid/Distribution.h"
#include "atlas-fesom/util/FesomDataFile.h"
#include "atlas-fesom/util/AtlasIOReader.h"

#include "atlas/runtime/Exception.h"

namespace atlas {
namespace grid {
namespace detail {
namespace partitioner {

FesomPartitioner::FesomPartitioner(const eckit::Parametrisation& config): Partitioner(config) {}

FesomPartitioner::FesomPartitioner(int N, const eckit::Parametrisation& config): Partitioner(N, config) {}

namespace {
void read_elements(const Grid& grid, std::vector<std::array<int64_t,3>>& connectivity_cell2node) {
    ATLAS_TRACE();
    std::string mpi_comm_self{"self"};
    fesom::AtlasIOReader read(grid.spec().getString("data"), mpi_comm_self);
    auto nb_cells = read.nb_cells();
    read.connectivity_cell2node(connectivity_cell2node);
    ATLAS_ASSERT(connectivity_cell2node.size() == nb_cells);
}
struct MetisGraph {
    metis::idx_t nvtxs = 0;
    metis::idx_t* xadj = nullptr;
    metis::idx_t* adjncy = nullptr;

    void assign(metis::idx_t _nvtxs, std::vector<metis::idx_t>&& _xadj, std::vector<idx_t>&& _adjncy ) {
        nvtxs = _nvtxs;
        xadj_ = std::move(_xadj);
        adjncy_ = std::move(_adjncy);
        xadj = xadj_.data();
        adjncy = adjncy_.data();
        metis_allocated_ = false;
    }

    ~MetisGraph() {
# if ATLAS_FESOM_HAVE_METIS
        if (metis_allocated_) {
            if (xadj) {
                ATLAS_ASSERT( METIS_Free(xadj) == METIS_OK );
            }
            if (adjncy) {
                ATLAS_ASSERT( METIS_Free(adjncy) == METIS_OK );
            }
        }
#endif
    }
private:
    bool metis_allocated_ = true;
    std::vector<metis::idx_t> xadj_;
    std::vector<metis::idx_t> adjncy_;
};

void build_node_graph(MetisGraph& graph, size_t nb_nodes, mdspan<const int64_t, extents<size_t,dynamic_extent,3>> connectivity_cell2node) {
    ATLAS_TRACE();
    std::vector<std::set<idx_t>> adjacency;
    adjacency.resize(nb_nodes);
    constexpr size_t elem_size = 3;
    for (size_t jelem=0; jelem < connectivity_cell2node.extent(0); ++jelem) {
        for (size_t i = 0; i < elem_size; ++i) {
            for (size_t j = 0; j < elem_size; ++j) {
                if (i != j) {
                    adjacency[connectivity_cell2node(jelem,i)].insert(connectivity_cell2node(jelem,j));
                }
            }
        }
    }

    // Convert to METIS format
    std::vector<metis::idx_t> xadj;
    std::vector<metis::idx_t> adjncy;
    metis::idx_t nvtxs = nb_nodes;
    xadj.resize(nb_nodes + 1);
    adjncy.reserve(6 * connectivity_cell2node.size()); // A good estmate: 3 edges per triangle, each edge counted twice (bidirectional)
    metis::idx_t edge_count = 0;
    for (size_t i = 0; i < nb_nodes; ++i) {
        xadj[i] = edge_count;
        for (auto neighbour : adjacency[i]) {
            adjncy.emplace_back(neighbour);
            edge_count++;
        }
    }
    xadj[nb_nodes] = edge_count;
    graph.assign(nvtxs, std::move(xadj), std::move(adjncy));
}

void build_node_graph(MetisGraph& graph, size_t nb_nodes, const std::vector<std::array<int64_t,3>>& connectivity_cell2node) {
    mdspan<const int64_t,extents<size_t,dynamic_extent,3>> elements_mdspan(connectivity_cell2node.data()->data(), connectivity_cell2node.size());
    build_node_graph(graph, nb_nodes, elements_mdspan);
}

void build_node_graph_using_metis(MetisGraph& graph, size_t nb_nodes, mdspan<const int64_t, extents<size_t,dynamic_extent,3>> connectivity_cell2node) {
#if ATLAS_FESOM_HAVE_METIS
    ATLAS_TRACE();
    size_t nb_elems = connectivity_cell2node.extent(0);
    constexpr size_t elem_size = 3;

    std::vector<metis::idx_t, pluto::host::allocator<metis::idx_t>> eind(elem_size*nb_elems);
    std::vector<metis::idx_t, pluto::host::allocator<metis::idx_t>> eptr(nb_elems+1);
    for (size_t jelem=0; jelem<nb_elems; ++jelem) {
        for (size_t jnode=0; jnode<elem_size; ++jnode) {
            eind[jelem * elem_size + jnode] = connectivity_cell2node(jelem,jnode);
        }
        eptr[jelem] = jelem * elem_size;
    }
    eptr[nb_elems] = nb_elems * elem_size;

    metis::idx_t ne = nb_elems;
    metis::idx_t nn = nb_nodes;
    metis::idx_t numflag = 0; // zero-based indexing of connectivity
    int status = METIS_MeshToNodal(&ne, &nn, eptr.data(), eind.data(), &numflag, &graph.xadj, &graph.adjncy);
    if (status != METIS_OK) {
        ATLAS_THROW_EXCEPTION("METIS_MeshToNodal failed!");
    }
    graph.nvtxs = nn;
#else
    build_node_graph(graph, nb_nodes, connectivity_cell2node);
#endif
}

void build_node_graph_using_metis(MetisGraph& graph, size_t nb_nodes, const std::vector<std::array<int64_t,3>>& connectivity_cell2node) {
    mdspan<const int64_t,extents<size_t,dynamic_extent,3>> elements_mdspan(connectivity_cell2node.data()->data(), connectivity_cell2node.size());
    build_node_graph_using_metis(graph, nb_nodes, elements_mdspan);
}


void partition_graph(MetisGraph& graph, int nb_partitions, int part[]) {
    ATLAS_TRACE();
    if (nb_partitions == 1) {
        for (size_t j=0; j<graph.nvtxs; ++j) {
            part[j] = 0;
        }
        return;
    }
#if ATLAS_FESOM_HAVE_METIS
    metis::idx_t options[METIS_NOPTIONS];
    ATLAS_ASSERT( METIS_SetDefaultOptions(options) == METIS_OK );
    options[METIS_OPTION_CONTIG]  = 1;    // try to make partitions contiguous
    options[METIS_OPTION_UFACTOR] = 30;   // imbalance tolerance (1-1000)
    metis::idx_t ncon = 1;
    metis::idx_t ec;
    metis::idx_t n = graph.nvtxs;
    metis::idx_t np = nb_partitions;
    static_assert( std::is_same_v<metis::idx_t, int> );
    int status = METIS_PartGraphKway(
        &graph.nvtxs,           // nvtxs
        &ncon,                  // ncon
        graph.xadj,      // xadj
        graph.adjncy,    // adjncy
        nullptr,  // vwgt
        nullptr,  // vsize
        nullptr,  // adjwgt
        &np,      // nparts
        nullptr,  // tpwgts
        nullptr,  // ubvec
        options,  // options
        &ec,      // edgecut
        part      // part
    );
    if (status != METIS_OK) {
        ATLAS_THROW_EXCEPTION("METIS_PartGraphKway failed!");
    }
#else
    ATLAS_THROW_EXCEPTION("atlas-fesom was compiled without METIS support; Use different partitioner or serial distribution");
#endif
}

}

void FesomPartitioner::partition(const Partitioner::Grid& grid, int part[]) const {
    ATLAS_TRACE();

    auto& comm = mpi::comm(mpi_comm());
    if (comm.rank() == 0) {
        std::vector<std::array<int64_t,3>> elements;
        read_elements(grid, elements);
        MetisGraph graph;
        // build_node_graph(graph, grid.size(), elements);
        build_node_graph_using_metis(graph, grid.size(), elements);
        partition_graph(graph, nb_partitions(), part);
    }
    comm.broadcast(part, grid.size(), 0);
}

}  // namespace partitioner
}  // namespace detail
}  // namespace grid
}  // namespace atlas

namespace {
atlas::grid::detail::partitioner::PartitionerBuilder<atlas::grid::detail::partitioner::FesomPartitioner> __Metis(
    atlas::grid::detail::partitioner::FesomPartitioner::static_type());
}
