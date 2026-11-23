#pragma once

import geometry;

namespace triangles {

bool doIntersect(const geometry::triangle_t<3>& first,
                 const geometry::triangle_t<3>& second);

} // namespace triangles
