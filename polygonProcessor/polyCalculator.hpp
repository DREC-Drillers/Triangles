#pragma once

#include <array>
#include <vector>

#include "intersectionData.hpp"
#include "polygons.hpp"

import geometry;

namespace triangles {

class PolyCalculator {
public:
    IntersectionData calculateIntersections(const Polygons& polys);

private:
    enum class trianglePosToPlane_t { INTERSECTS, BELONGS, DOES_NOT_INTERSECT };

    struct planeRelativeVertex_t {
        geometry::point_t vertex;
        double orient;
    };

    struct planeRelativeTriangle_t {
        std::array<planeRelativeVertex_t, 3> vertices;
        trianglePosToPlane_t relation;
    };

    static planeRelativeTriangle_t
    getTrianglePosToPlaneOfTriangle(const geometry::triangle_t& curTriangle,
                                    const geometry::triangle_t& planeTriangle);

    static std::size_t
    findIsolatedVertexIndex(const planeRelativeTriangle_t& triangle);

    static void canonicalizeTriangle(planeRelativeTriangle_t& triangle,
                                     planeRelativeTriangle_t& other);

    static bool GuigueDevillers(const geometry::triangle_t& first,
                                const geometry::triangle_t& second);
};

} // namespace triangles
