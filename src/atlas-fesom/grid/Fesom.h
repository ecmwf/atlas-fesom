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

    Fesom(const std::string& name, const std::string& uid, size_t N, double lon[], double lat[], size_t lon_stride = 1, size_t lat_stride = 1) :
        Unstructured(N,lon,lat,lon_stride,lat_stride) {
        name_ = name;
        uid_ = uid;
    }
    static std::string static_type() { return "fesom"; }
    std::string name() const override { return name_; }
    std::string type() const override { return static_type(); }
    std::string uid() const override { return uid_; }
    Config meshgenerator() const override {
        return Config("type", "fesom");
}

private:
    std::string name_;
    std::string uid_;
};


}  // namespace grid
}  // namespace detail
}  // namespace grid
}  // namespace atlas
