#pragma once

#include <vector>

import geometry;

namespace triangles {

struct InitPolysData {
    using PolygonPoints = std::vector<geometry::point_t<3>>;

    std::vector<PolygonPoints> data;
};

} // namespace triangles
