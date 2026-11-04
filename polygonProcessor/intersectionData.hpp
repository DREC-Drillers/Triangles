#pragma once

#include <cstddef>
#include <vector>

import geometry;

namespace triangles {

struct PolyIntersection {
    enum class Kind { POINT, SEGMENT, POLYGON };

    Kind kind;
    // One point, two segment endpoints, or polygon vertices in boundary order.
    // The polygon's first vertex is not repeated at the end.
    std::vector<geometry::point_t<3>> points;
};

template <std::size_t dimWorld>
class PolyWorld;

struct IntersectionData {
    const PolyWorld<3>& world;
    std::vector<const geometry::polygon_t<3>*> intersectingPolys;
    std::vector<const geometry::polygon_t<3>*> nonIntersectingPolys;
};

} // namespace triangles
