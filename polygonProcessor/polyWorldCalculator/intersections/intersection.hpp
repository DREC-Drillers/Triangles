#pragma once

import geometry;

namespace triangles {

enum class RealTriangleForm { POINT, SEGMENT, TRIANGLE };

RealTriangleForm determineRealTriangleForm(const geometry::triangle_t<3>& triangle);

} // namespace triangles
