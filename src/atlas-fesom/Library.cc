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
#include <vector>

#include "eckit/config/Resource.h"
#include "eckit/filesystem/PathName.h"
#include "eckit/filesystem/LocalPathName.h"
#include "eckit/runtime/Main.h"
#include "eckit/utils/Tokenizer.h"

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

    std::vector<eckit::PathName> grids_paths;
    {
        eckit::Tokenizer tokenize(":");
        std::vector<std::string> tokenized;
        tokenize( gridsPath(), tokenized);
        for( auto& t : tokenized ) {
            if( not t.empty() ) {
                grids_paths.push_back(t);
            }
        }
    }

    for (const auto& grids_path: grids_paths) {
        if (grids_path.exists()) {
            std::vector<eckit::PathName> grids_files;
            if (grids_path.isDir()) {
                std::vector<eckit::PathName> dirs;
                grids_path.children( grids_files, dirs );
            }
            else {
                grids_files.push_back( grids_path );
            }
            {
                // In case the grids_path is in the Build dir, there are files to be filtered
                std::vector<eckit::PathName> filtered;
                filtered.reserve(grids_files.size());
                for (const auto& grids_file: grids_files) {
                    bool keep = true;
                    std::string basename = grids_file.baseName();
                    if (basename == "Makefile" || basename == "CMakeLists.txt" || basename.find(".cmake") != std::string::npos) {
                        keep = false;
                    }
                    if (keep) {
                        filtered.emplace_back(grids_file);
                    }
                }
                grids_files.swap(filtered);
            }

            for (const auto& grids_file: grids_files) {
                Log::debug() << "Plugin atlas-fesom loading grids from file " << grids_file << std::endl;
                auto grids = util::Config( grids_file);
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
                        if (id != spec.getString("uid","")) {
                            grid::GridBuilder& grid_builder = *grid::GridBuilder::typeRegistry().at("fesom");
                            grid_builder.registerNamedGrid(id);
                        }
                    }
                }
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
    eckit::PathName cwd = eckit::LocalPathName::cwd();
    return atlas::Library::instance().dataPath()+":~atlas-fesom/share:"+cwd.asString();
}

std::string Library::cachePath() const {
    return atlas::Library::instance().cachePath();
}

std::string Library::gridsPath() const {
    static std::string ATLAS_FESOM_GRIDS_PATH = eckit::LibResource<std::string, atlas::fesom::Library>(
        "atlas-fesom-grids-path;$ATLAS_FESOM_GRIDS_PATH", "" );
    return ATLAS_FESOM_GRIDS_PATH + ":~atlas-fesom/share/atlas/grids/fesom" + ":atlas-fesom-grids.yaml";
}

}  // namespace fesom
}  // namespace atlas
