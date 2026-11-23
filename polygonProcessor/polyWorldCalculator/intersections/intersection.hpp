#pragma once

#include "spatial/geometry/geomConst.hpp"

import geometry;

namespace triangles {

bool doIntersect(const geometry::triangle_t<geometry::defaultDim>& first,
                 const geometry::triangle_t<geometry::defaultDim>& second);

} // namespace triangles
