
#include "polyCalculator.hpp"

namespace triangles {

std::optional<PolyIntersection>
PolyCalculator::arePolyIntersect(const polygon_t& first, const polygon_t& second) {

}

bool
PolyCalculator::GuigueDevillers(const triangle_t& first, const triangle_t& second) {
    const auto firstPos = getTrianglePosToPlaneOfTriangle(first, second);
    if (firstPos.relation == trianglePosToPlane_t::DOES_NOT_INTERSECT) {
        return false;
    }
    if (firstPos.relation == trianglePosToPlane_t::BELONGS) {
        return isTrianglesIntersect2D(first, second);
    }

    const auto secondPos = getTrianglePosToPlaneOfTriangle(second, first);
    if (secondPos.relation == trianglePosToPlane_t::DOES_NOT_INTERSECT) {
        return false;
    }





}

planeRelativeTriangle_t getTrianglePosToPlaneOfTriangle(
    const geometry::triangle_t& curTriangle,
    const geometry::triangle_t& planeTriangle) {

    const double firstVertexOrient = geometry::point_t::orient3D(
        planeTriangle.a(), planeTriangle.b(), planeTriangle.c(), curTriangle.a());
    const double secondVertexOrient = geometry::point_t::orient3D(
        planeTriangle.a(), planeTriangle.b(), planeTriangle.c(), curTriangle.b());
    const double thirdVertexOrient = geometry::point_t::orient3D(
        planeTriangle.a(), planeTriangle.b(), planeTriangle.c(), curTriangle.c());

    planeRelativeTriangle_t result{
        {{{curTriangle.a(), firstVertexOrient},
          {curTriangle.b(), secondVertexOrient},
          {curTriangle.c(), thirdVertexOrient}}},
        trianglePosToPlane_t::DOES_NOT_INTERSECT
    };

    constexpr double epsilon = 1e-9;
    bool hasPositive = false;
    bool hasNegative = false;
    bool hasZero = false;

    for (double orientation : {firstVertexOrient, secondVertexOrient, thirdVertexOrient}) {
        if (orientation > epsilon) {
            hasPositive = true;
        } else if (orientation < -epsilon) {
            hasNegative = true;
        } else {
            hasZero = true;
        }
    }

    if (!hasPositive && !hasNegative) {
        result.relation = trianglePosToPlane_t::BELONGS;
    } else if (hasZero || (hasPositive && hasNegative)) {
        result.relation = trianglePosToPlane_t::INTERSECTS;
    }
    return result;
}

} // namespace triangles
