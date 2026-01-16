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

#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include "eckit/filesystem/PathName.h"

#include "atlas/array/DataType.h"
#include "atlas/util/Config.h"

namespace atlas {
namespace fesom {

//------------------------------------------------------------------------------------------------------

class FesomData {
public:
    std::uint64_t nb_nodes;
    std::uint64_t nb_cells;
    std::vector<double> lon;
    std::vector<double> lat;
    std::vector<std::int64_t> connectivity_cell2node;

    void checkSetup();

    size_t write( const eckit::PathName& path, const util::Config& config );

    std::string computeUid(std::string arrangement);

    void ensureOutwardNormals();

};

//------------------------------------------------------------------------------------------------------

}  // namespace fesom
}  // namespace atlas
