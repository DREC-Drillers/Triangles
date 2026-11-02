#pragma once

#include <vector>

import geometry;

namespace triangles {

struct InitPolysData {
    using PolygonPoints = std::vector<geometry::point_t>;

    std::vector<PolygonPoints> data;
};

} // namespace triangles
