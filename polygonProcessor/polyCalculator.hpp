#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "intersectionData.hpp"

import geometry

namespace triangles {

class PolyCalculator {
public:
    IntersectionData calculateIntersections(const std::vector<polygon_t>& polys);

private:
    std::optional<PolyIntersection> arePolyIntersect(const polygon_t& first,
                                                     const polygon_t& second);

    enum class trianglePosToPlane_t { INTERSECTS, BELONGS, DOES_NOT_INTERSECT };

    struct planeRelativeVertex_t {
        point_t vertex;
        double orient;
    }

    struct planeRelativeTriangle_t{
        std::array<planeRelativeVertex_t, 3> vertices;
        trianglePosToPlane_t relation;
    };
};

} // namespace triangles
