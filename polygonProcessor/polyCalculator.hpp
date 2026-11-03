#pragma once

#include <optional>
#include <vector>

#include "intersectionData.hpp"

import geometry;

namespace triangles {

class PolyCalculator {
public:
    // Collect all intersecting pairs and their intersection geometry.
    IntersectionData
    calculateIntersections(const std::vector<geometry::polygon_t>& polys);

private:
    // std::nullopt means the pair does not intersect.
    std::optional<PolyIntersection>
    intersectsPoly(const geometry::polygon_t& first,
                   const geometry::polygon_t& second);
};

} // namespace triangles
