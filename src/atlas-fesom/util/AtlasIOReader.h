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
#include "eckit/log/ProgressTimer.h"

#include "atlas/io/atlas-io.h"
#include "atlas/runtime/Exception.h"

#include "atlas-fesom/util/FesomDataFile.h"

namespace atlas {
namespace fesom {

class AtlasIOReader {
public:
    AtlasIOReader( const std::string& uri ) :
        reader_(FesomDataFile(uri)){
        reader_.read( "version", version_ ).wait();
    }

    std::uint64_t nb_nodes() {
        std::uint64_t value;
        reader_.read("nb_nodes",value).wait();
        return value;
    }

    std::uint64_t nb_cells() {
        std::uint64_t value;
        reader_.read("nb_cells",value).wait();
        return value;
    }

    template<typename Value>
    void nb_nodes(Value& value) {
        value = nb_nodes();
    }

    template<typename Value>
    void nb_cells(Value& value) {
        value = nb_cells();
    }

    void longitude(std::vector<double>& value) {
        reader_.read("longitude",value).wait();
    }

    void latitude(std::vector<double>& value) {
        reader_.read("latitude",value).wait();
    }

    void connectivity_cell2node(std::vector<std::array<std::int64_t,3>>& value) {
        reader_.read("connectivity_cell2node",value).wait();
    }

private:
    io::RecordReader reader_;
    int version_;
};

}  // namespace fesom
}  // namespace atlas

//------------------------------------------------------------------------------------------------------
