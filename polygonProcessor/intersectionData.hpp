#pragma once

#include <cstddef>
#include <utility>
#include <vector>

namespace triangles {

// Indices into the input Polygons collection; no geometry is copied or owned.
using IntersectingPair = std::pair<std::size_t, std::size_t>;

using IntersectionData = std::vector<IntersectingPair>;

} // namespace triangles
