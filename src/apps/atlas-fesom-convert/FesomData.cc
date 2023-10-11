/*
 * (C) Copyright 2021- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "FesomData.h"

#include "atlas/io/atlas-io.h"

#include "atlas/util/Geometry.h"
#include "atlas/util/Point.h"

#include "atlas-fesom/util/ComputeUid.h"

namespace atlas {
namespace fesom {

std::string atlas::fesom::FesomData::computeUid( const util::Config& config ) {
    size_t size = lon.size();
    return fesom::compute_uid( lon.data(), lat.data(), size );
}

void atlas::fesom::FesomData::checkSetup() {
    ATLAS_ASSERT( nb_nodes >= 0 );
    ATLAS_ASSERT( not lon.empty() );
    ATLAS_ASSERT( not lat.empty() );
    ATLAS_ASSERT( lon.size() == nb_nodes );
    ATLAS_ASSERT( lat.size() == nb_nodes );
}

size_t atlas::fesom::FesomData::write( const eckit::PathName& path, const util::Config& config ) {
    checkSetup();
    io::RecordWriter record;
    record.compression( config.getString( "compression", "none" ) );
    record.set( "version", 0 );
    record.set( "nb_nodes", io::ref( nb_nodes ) );
    record.set( "nb_cells", io::ref( nb_cells ) );
    record.set( "longitude", io::ArrayReference( lon.data(), {nb_nodes} ) );
    record.set( "latitude", io::ArrayReference( lat.data(), {nb_nodes} ) );
    record.set( "connectivity_cell2node", io::ArrayReference( connectivity_cell2node.data(), io::ArrayShape{nb_cells,std::uint64_t(3)} ) );
    return record.write( path );
}

void FesomData::ensureOutwardNormals() {
    Geometry geometry("UnitSphere");

    auto ensure_outward_normal = [&](size_t n) {
        auto a = geometry.xyz( PointLonLat{lon[connectivity_cell2node[n*3+0]], lat[connectivity_cell2node[n*3+0]]} );
        auto b = geometry.xyz( PointLonLat{lon[connectivity_cell2node[n*3+1]], lat[connectivity_cell2node[n*3+1]]} );
        auto c = geometry.xyz( PointLonLat{lon[connectivity_cell2node[n*3+2]], lat[connectivity_cell2node[n*3+2]]} );

        auto dot = [](const auto& p1, const auto& p2) {
            return p1[0]*p2[0] + p1[1]*p2[1] + p1[2]*p2[2];
        };
        auto cross = [](const auto& p1, const auto& p2) {
            return std::array<double,3>{
                p1[1] * p2[2] - p1[2] * p2[1], p1[2] * p2[0] - p1[0] * p2[2],
                p1[0] * p2[1] - p1[1] * p2[0]
            };
        };

        std::array<double,3> ba {a[0]-b[0], a[1]-b[1], a[2]-b[2]};
        std::array<double,3> bc {c[0]-b[0], c[1]-b[1], c[2]-b[2]};

        bool outward = dot(b, cross(bc,ba)) > 0;

        if (not outward) {
            std::swap(connectivity_cell2node[n*3+1], connectivity_cell2node[n*3+2]);
        }
        return outward;
    };

    for ( size_t n = 0; n < nb_cells; ++n ) {
        ensure_outward_normal(n);
    }

}

}  // namespace fesom
}  // namespace atlas
