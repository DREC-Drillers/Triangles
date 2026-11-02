#pragma once

#include <utility>
#include <vector>

import geometry;

namespace triangles {

using IntersectingPair = std::pair<polygon_t, polygon_t>;

using IntersectionData = std::vector<IntersectingPair>;

} // namespace triangles
