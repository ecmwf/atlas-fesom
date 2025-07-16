/*
 * (C) Copyright 2021- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "Fesom.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numeric>

#include "atlas-fesom/Library.h"

#include "atlas/grid/SpecRegistry.h"
#include "atlas/grid/detail/grid/GridBuilder.h"
#include "atlas/grid/detail/grid/GridFactory.h"
#include "atlas-fesom/grid/Fesom.h"
#include "atlas-fesom/util/FesomDataFile.h"
#include "atlas-fesom/util/AtlasIOReader.h"
#include "atlas/library.h"
#include "atlas/util/Config.h"

namespace atlas {
namespace grid {
namespace detail {
namespace grid {

static class FesomGridBuilder : public GridBuilder {
    using Implementation = atlas::Grid::Implementation;
    using Config         = Grid::Config;

public:
    FesomGridBuilder() : GridBuilder( FesomNodes::static_type() ) {}

    const std::string& type() const override {
        static std::string _type{"unstructured"};
        return _type;
    }

    void print( std::ostream& os ) const override {
        os << std::left << std::setw( 30 ) << "FESOM<N>"
           << "FESOM unstructured ocean grid. Possible increasing resolutions <deg>: 2,1,025,12";
    }

    const Implementation* create( const std::string& name_or_uid, const Config& /* config */ ) const override {
        auto sane_id( name_or_uid );

        if ( SpecRegistry::has( sane_id ) ) {
            return create( SpecRegistry::get( sane_id ) );
        }

        std::transform( sane_id.begin(), sane_id.end(), sane_id.begin(), ::tolower );

        if ( SpecRegistry::has( sane_id ) ) {
            return create( SpecRegistry::get( sane_id ) );
        }

        std::transform( sane_id.begin(), sane_id.end(), sane_id.begin(), ::toupper );

        if ( SpecRegistry::has( sane_id ) ) {
            return create( SpecRegistry::get( sane_id ) );
        }

        return nullptr;
    }

    const Implementation* create( const Config& config ) const override {
        std::string type;
        config.get("type",type);
        if (type != "FESOM") {
            return nullptr;
        }

        std::string name = config.getString("name");
        std::string uid = config.getString("uid");
        std::string functionspace = config.getString("functionspace");
        
        fesom::AtlasIOReader read(fesom::FesomDataFile(config.getString("data")));
        std::vector<double> lon;
        std::vector<double> lat;
        std::size_t nb_nodes{0};
        read.nb_nodes(nb_nodes);
        read.longitude(lon);
        read.latitude(lat);
        if (functionspace == "nodes") {
            return new FesomNodes(name, uid, nb_nodes, lon.data(), lat.data());
        }
        else if (functionspace == "cells") {
            double cyclic_length = 360.;
            std::size_t nb_cells{0};
            std::vector<std::array<std::int64_t,3>> triangles;
            read.nb_cells(nb_cells);
            read.connectivity_cell2node(triangles);
            ATLAS_ASSERT(triangles.size() == nb_cells);
            std::vector<double> clon(nb_cells);
            std::vector<double> clat(nb_cells);
            size_t j{0};
            for (const auto& triangle: triangles) {
                std::array<double,3> triangle_lons{lon[triangle[0]], lon[triangle[1]], lon[triangle[2]]};
                std::array<double,3> triangle_lats{lat[triangle[0]], lat[triangle[1]], lat[triangle[2]]};
                const double lon_min = *std::min_element(triangle_lons.begin(), triangle_lons.end());
                for (auto& l: triangle_lons) {
                    if (l - lon_min > cyclic_length*0.5) {
                        l -= cyclic_length;
                    }
                }
                clon[j] = std::accumulate(triangle_lons.begin(),triangle_lons.end(),0.) / triangle_lons.size();
                clat[j] = std::accumulate(triangle_lats.begin(),triangle_lats.end(),0.) / triangle_lats.size();
                ++j;
            }
            return new FesomCentroids(name, uid, nb_cells, clon.data(), clat.data());
        }
        ATLAS_THROW_EXCEPTION("Unrecognised value for key 'functionspace': " << functionspace);
    }

    void force_link() {}

} fesom_;

}  // namespace grid
}  // namespace detail
}  // namespace grid
}  // namespace atlas
