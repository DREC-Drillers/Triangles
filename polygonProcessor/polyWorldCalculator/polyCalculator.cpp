#include "polyCalculator.hpp"

#include <stdexcept>

#include "polygonProcessor/polyWorldCalculator/intersections/triangleBounds.hpp"
#include "polygonProcessor/polyWorldCalculator/intersections/triangleIntersection.hpp"

namespace triangles {

bool PolyCalculator::intersects(const geometry::polygon_t<3>& first,
                                const geometry::polygon_t<3>& second) const {
    if (first.numOfPoints() != 3 || second.numOfPoints() != 3) {
        throw std::invalid_argument("Only triangles are supported; other "
                                    "polygons are not supported yet");
    }

    const geometry::triangle_t<3> firstTriangle(
        first.point(0), first.point(1), first.point(2));
    const geometry::triangle_t<3> secondTriangle(
        second.point(0), second.point(1), second.point(2));
    return intersects(firstTriangle, secondTriangle);
}

bool PolyCalculator::intersects(const geometry::triangle_t<3>& first,
                                const geometry::triangle_t<3>& second) const {
    if (!triangleBounds(first).overlaps(triangleBounds(second))) {
        return false;
    }

    return doTrianglesIntersect(first, second);
}

} // namespace triangles
