/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#pragma once

#include "atlas/grid/detail/partitioner/Partitioner.h"

#include "atlas/grid/Distribution.h"
#include "atlas/grid/Grid.h"

namespace atlas {
namespace grid {
namespace detail {
namespace partitioner {

class FesomPartitioner : public Partitioner {
public:
    FesomPartitioner(const eckit::Parametrisation& config = util::NoConfig());
    FesomPartitioner(int N, const eckit::Parametrisation& config);

    std::string type() const override { return static_type(); }
    static std::string static_type() { return "fesom"; }

    using Partitioner::partition;
    void partition(const Grid&, int part[]) const override;
};

}  // namespace partitioner
}  // namespace detail
}  // namespace grid
}  // namespace atlas
