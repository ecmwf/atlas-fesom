/*
 * (C) Copyright 2021- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#pragma once

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>


#include "eckit/filesystem/PathName.h"

#include "atlas/io/atlas-io.h"
#include "atlas/runtime/Exception.h"

#include "atlas-fesom/util/FesomDataFile.h"

namespace atlas {
namespace fesom {

class AtlasIOReader {
public:
    AtlasIOReader( const util::Config& config = util::NoConfig() ) {}

    void read( const std::string& uri, FesomData& data ) {
        FesomDataFile file{uri};

        io::RecordReader reader( file );

        int version;
        reader.read( "version", version ).wait();
        if ( version == 0 ) {
            reader.read( "nb_nodes", data.nb_nodes );
            reader.read( "nb_cells", data.nb_cells );
            reader.read( "longitude", data.lon );
            reader.read( "latitude", data.lat );
            reader.read( "connectivity_cell2node", data.connectivity_cell2node );
            reader.wait();
        }
        else {
            ATLAS_THROW_EXCEPTION( "Unsupported version " << version );
        }
    }

    AtlasIOReader( const std::string& uri ) {}

};

}  // namespace fesom
}  // namespace atlas

//------------------------------------------------------------------------------------------------------
