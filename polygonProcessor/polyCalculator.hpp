#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

#include "intersectionData.hpp"

import geometry;

namespace triangles {

enum class trianglePosToPlane_t { INTERSECTS, BELONGS, DOES_NOT_INTERSECT };

struct planeRelativeVertex_t {
    geometry::point_t vertex;
    double orient;
};

struct planeRelativeTriangle_t {
    std::array<planeRelativeVertex_t, 3> vertices;
    trianglePosToPlane_t relation;
};

planeRelativeTriangle_t getTrianglePosToPlaneOfTriangle(
    const geometry::triangle_t& curTriangle,
    const geometry::triangle_t& planeTriangle);

class PolyCalculator {
public:
    IntersectionData calculateIntersections(const std::vector<polygon_t>& polys);

private:
    std::optional<PolyIntersection> arePolyIntersect(const polygon_t& first,
                                                     const polygon_t& second);
};

} // namespace triangles
