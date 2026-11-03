#pragma once

#include <array>
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

// Both triangles must have relation INTERSECTS.
void canonicalizePlaneRelativeTriangles(planeRelativeTriangle_t& first,
                                       planeRelativeTriangle_t& second);

class PolyCalculator {
public:
    IntersectionData calculateIntersections(const std::vector<geometry::polygon_t>& polys);

private:
    bool GuigueDevillers(const geometry::triangle_t& first,
                         const geometry::triangle_t& second);
};

} // namespace triangles
