/*
 * (C) Copyright 2021- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <algorithm> // std::transform
#include <cctype>    // ::tolower
#include <string>

#include "eckit/config/Resource.h"
#include "eckit/filesystem/PathName.h"
#include "eckit/runtime/Main.h"
#include "atlas-fesom/Library.h"
#include "atlas-fesom/version.h"
#include "atlas/grid/SpecRegistry.h"
#include "atlas/library/Library.h"
#include "atlas/grid/detail/grid/GridBuilder.h"

namespace atlas {
class Grid;
}


namespace atlas {
namespace fesom {


REGISTER_LIBRARY( Library );


Library::Library() : Plugin( "atlas-fesom" ) {}


const Library& Library::instance() {
    static Library library;
    return library;
}


std::string Library::version() const {
    return atlas_fesom_version();
}


std::string Library::gitsha1( unsigned int count ) const {
    std::string sha1 = atlas_fesom_git_sha1();
    return sha1.empty() ? "not available" : sha1.substr( 0, std::min( count, 40U ) );
}


void Library::init() {
    Plugin::init();
    auto grids = util::Config( gridsPath() );
    for ( auto& id : grids.keys() ) {
        auto spec = grids.getSubConfiguration(id);
        std::string type = spec.getString("type","");
        std::transform( type.begin(), type.end(), type.begin(), ::tolower );
        if (type == "fesom") {
            Log::debug() << "Plugin atlas-fesom registering grid " << id << std::endl;
            if(!spec.has("name")) {
                ATLAS_ASSERT(spec.has("base_name"));
                ATLAS_ASSERT(spec.has("arrangement"));
                spec.set("name", spec.getString("base_name")+"_"+spec.getString("arrangement"));
            }
            grid::SpecRegistry::add(id, spec);
            if (id != spec.getString("uid")) {
                grid::GridBuilder& grid_builder = *grid::GridBuilder::typeRegistry().at("fesom");
                grid_builder.registerNamedGrid(id);
            }
        }
    }
}

bool Library::caching() const {
    static bool ATLAS_FESOM_CACHING =
        bool( eckit::LibResource<bool, atlas::fesom::Library>( "atlas-fesom-caching;$ATLAS_FESOM_CACHING", false ) );
    return ATLAS_FESOM_CACHING;
}

std::string Library::dataPath() const {
    return atlas::Library::instance().dataPath()+":~atlas-fesom/share";
}

std::string Library::cachePath() const {
    return atlas::Library::instance().cachePath();
}

std::string Library::gridsPath() const {
    static std::string ATLAS_FESOM_GRIDS_PATH = eckit::LibResource<std::string, atlas::fesom::Library>(
        "atlas-fesom-grids-path;$ATLAS_FESOM_GRIDS_PATH", "" );
    ATLAS_ASSERT( not ATLAS_FESOM_GRIDS_PATH.empty() );
    return ATLAS_FESOM_GRIDS_PATH;
}

}  // namespace fesom
}  // namespace atlas
