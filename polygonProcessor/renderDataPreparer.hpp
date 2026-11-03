#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "../render/renderData.hpp"
#include "intersectionData.hpp"

import geometry;

namespace triangles {

struct RenderStyle {
    std::array<float, 4> polyColor{0.7f, 0.7f, 0.7f, 1.0f};
    std::array<float, 4> intersectingPolyColor{1.0f, 0.0f, 0.0f, 1.0f};
    std::array<float, 4> intersectionColor{1.0f, 0.8f, 0.0f, 1.0f};
};

// Adapts domain geometry to the renderer's fixed 3D format.
template <std::size_t dimWorld>
class RenderDataPreparer {
public:
    // objects: triangulated polygons with normals; intersecting polygons are red.
    // highlights: POINT -> POINTS, SEGMENT -> LINES, POLYGON -> TRIANGLES.
    // Also emit every intersection vertex as POINTS in intersectionColor.
    // Polygon intersections require triangulation, not just copying their vertices.
    RenderData prepare(const std::vector<geometry::polygon_t>& polys,
                       const IntersectionData& intersections,
                       const RenderStyle& style) const;
};

} // namespace triangles
