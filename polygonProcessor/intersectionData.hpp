#pragma once

#include <vector>

#include "polyWorld.hpp"

namespace triangles {

struct IntersectionData {
    const PolyWorld<3>& world;
    std::vector<const geometry::polygon_t*> intersectingPolys;
    std::vector<const geometry::polygon_t*> nonIntersectingPolys;
};

} // namespace triangles
