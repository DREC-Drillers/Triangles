#pragma once

#include <vector>

#include "spatial/geometry/geomConst.hpp"

import geometry;

namespace triangles {

struct InitPolysData {
    using PolygonPoints = std::vector<geometry::point_t<geometry::defaultDim>>;

    std::vector<PolygonPoints> data;
};

} // namespace triangles
