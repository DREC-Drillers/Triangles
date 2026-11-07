#pragma once

#include <vector>

#include "polygonProcessor/polyWorld.hpp"

namespace triangles {

struct IntersectionData {
    const PolyWorld<3>& world;
    std::vector<const geometry::polygon_t<3>*> intersectingPolys;
    std::vector<const geometry::polygon_t<3>*> nonIntersectingPolys;
};

} // namespace triangles
