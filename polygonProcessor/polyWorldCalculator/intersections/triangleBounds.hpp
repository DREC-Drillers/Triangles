#pragma once

import geometry;

namespace triangles {

geometry::axisAlignedBoundingBox_t
triangleBounds(const geometry::triangle_t<3>& triangle);

} // namespace triangles
