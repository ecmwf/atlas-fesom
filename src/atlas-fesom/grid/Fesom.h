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

#include <cstddef>
#include <memory>
#include <vector>

#include "atlas/grid/detail/grid/Unstructured.h"
#include "atlas/runtime/Exception.h"
#include "atlas/util/Config.h"
#include "atlas/util/Point.h"

namespace atlas {
class Mesh;
}  // namespace atlas
namespace eckit {
class PathName;
}

namespace atlas {
namespace grid {
namespace detail {
namespace grid {

class Fesom : public Unstructured {
public:

    enum class Arrangement {
        N,
        C
    };

    Fesom(const std::string& uid, Arrangement arrangement, size_t N, double lon[], double lat[], size_t lon_stride = 1, size_t lat_stride = 1) :
        Unstructured(uid, N,lon,lat,lon_stride,lat_stride) {
        arrangement_ = arrangement;
    }
    static std::string static_type() { return "fesom"; }
    std::string type() const override { return static_type(); }

    Config meshgenerator() const override {
        switch (arrangement_) {
            case Arrangement::N: return Config("type", "fesom");
            default: return Unstructured::meshgenerator();
        }
    }
    Config partitioner() const override {
        switch (arrangement_) {
            case Arrangement::N: return Unstructured::partitioner();
            default: return Unstructured::partitioner();
        }
    }
    Arrangement arrangement() const {
        return arrangement_;
    }

private:
    Arrangement arrangement_;
};


}  // namespace grid
}  // namespace detail
}  // namespace grid
}  // namespace atlas
