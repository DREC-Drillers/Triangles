#pragma once

#include <vector>

#include "spatial/geometry/geomConst.hpp"

import geometry;

namespace triangles {

struct IntersectionData {
    std::vector<const geometry::polygon_t<geometry::defaultDim>*> intersectingPolys;
    std::vector<const geometry::polygon_t<geometry::defaultDim>*>
        nonIntersectingPolys;
};

} // namespace triangles
