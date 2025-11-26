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

class FesomNodes : public Unstructured {
public:

    FesomNodes(const std::string& uid, size_t N, double lon[], double lat[], size_t lon_stride = 1, size_t lat_stride = 1) :
        Unstructured(uid, N,lon,lat,lon_stride,lat_stride) {
    }
    static std::string static_type() { return "fesom"; }
    std::string type() const override { return static_type(); }
    Config meshgenerator() const override {
        return Config("type", "fesom");
}

};


class FesomCentroids : public Unstructured {
public:

    FesomCentroids(const std::string& uid, size_t N, double lon[], double lat[], size_t lon_stride = 1, size_t lat_stride = 1) :
        Unstructured(uid, N,lon,lat,lon_stride,lat_stride) {
    }
    static std::string static_type() { return "fesom"; }
    std::string type() const override { return static_type(); }
    Config meshgenerator() const override {
        return Config("type", "delaunay");
    }

};


}  // namespace grid
}  // namespace detail
}  // namespace grid
}  // namespace atlas
