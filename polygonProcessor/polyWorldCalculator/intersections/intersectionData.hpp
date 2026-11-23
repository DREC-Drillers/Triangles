#pragma once

#include <vector>

import geometry;

namespace triangles {

struct IntersectionData {
    std::vector<const geometry::polygon_t<3>*> intersectingPolys;
    std::vector<const geometry::polygon_t<3>*> nonIntersectingPolys;
};

} // namespace triangles
