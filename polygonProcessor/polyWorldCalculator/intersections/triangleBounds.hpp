#pragma once

#include "polygonProcessor/polyWorldCalculator/spatial/aabb.hpp"

import geometry;

namespace triangles {

Aabb triangleBounds(const geometry::triangle_t<3>& triangle);

} // namespace triangles
