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
    FesomGridBuilder() : GridBuilder( Fesom::static_type(), {"^FESOM[0-9]"}, {"FESOM<N>"} ) {}

    void print( std::ostream& os ) const override {
        os << std::left << std::setw( 30 ) << "FESOM<N>"
           << "FESOM Tripolar grid. Possible increasing resolutions <deg>: 2,1,025,12";
    }

    const Implementation* create( const std::string& name_or_uid, const Config& /* config */ ) const override {
        auto sane_id( name_or_uid );
        std::transform( sane_id.begin(), sane_id.end(), sane_id.begin(), ::tolower );

        if ( SpecRegistry::has( sane_id ) ) {
            return create( SpecRegistry::get( sane_id ) );
        }

        auto sane_name( name_or_uid );
        std::transform( sane_name.begin(), sane_name.end(), sane_name.begin(), ::toupper );

        if ( SpecRegistry::has( sane_name ) ) {
            return create( SpecRegistry::get( sane_name ) );
        }

        return nullptr;
    }

    const Implementation* create( const Config& config ) const override {

        std::string name = config.getString("name");
        std::string uid = config.getString("uid");

        fesom::AtlasIOReader read(fesom::FesomDataFile(config.getString("data")));
        std::vector<double> lon;
        std::vector<double> lat;
        read.longitude(lon);
        read.latitude(lat);
        auto nb_nodes = read.nb_nodes();
        return new Fesom(name, uid, nb_nodes, lon.data(), lat.data());
    }

    void force_link() {}

} fesom_;

}  // namespace grid
}  // namespace detail
}  // namespace grid
}  // namespace atlas
