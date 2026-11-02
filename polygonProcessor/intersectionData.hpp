#pragma once

#include <utility>
#include <vector>

import geometry;

namespace triangles {

using IntersectingPair = std::pair<geometry::polygon_t, geometry::polygon_t>;

using IntersectionData = std::vector<IntersectingPair>;

} // namespace triangles
