
#include "polyCalculator.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace triangles {

using geometry::point_t;
using geometry::triangle_t;
using geometry::vector_t;

namespace {

constexpr double orientationEpsilon = 1e-9;

int orientationSign(double orientation) {
    if (orientation > orientationEpsilon)
        return 1;
    if (orientation < -orientationEpsilon)
        return -1;
    return 0;
}

struct Point2D {
    double x;
    double y;
};

Point2D projectPoint(const point_t& point, std::size_t droppedAxis) {
    if (droppedAxis == 0)
        return {point.y(), point.z()};
    if (droppedAxis == 1)
        return {point.x(), point.z()};
    return {point.x(), point.y()};
}

double orient2D(const Point2D& a, const Point2D& b, const Point2D& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool hasSeparatingEdge(const std::array<Point2D, 3>& first,
                       const std::array<Point2D, 3>& second) {
    for (std::size_t i = 0; i < 3; ++i) {
        const auto& a = first[i];
        const auto& b = first[(i + 1) % 3];
        const int interior = orientationSign(orient2D(a, b, first[(i + 2) % 3]));
        bool allOutside = true;
        for (const auto& point : second) {
            if (orientationSign(orient2D(a, b, point)) != -interior ||
                interior == 0) {
                allOutside = false;
                break;
            }
        }
        if (allOutside)
            return true;
    }
    return false;
}

bool isTrianglesIntersect2D(const triangle_t& first, const triangle_t& second) {
    const auto normal =
        vector_t(first.a(), first.b()).cross(vector_t(first.a(), first.c()));
    const std::array<double, 3> components{
        std::abs(normal.x()), std::abs(normal.y()), std::abs(normal.z())};
    const std::size_t droppedAxis =
        std::max_element(components.begin(), components.end()) - components.begin();
    const std::array<Point2D, 3> projectedFirst{
        projectPoint(first.a(), droppedAxis),
        projectPoint(first.b(), droppedAxis),
        projectPoint(first.c(), droppedAxis)};
    const std::array<Point2D, 3> projectedSecond{
        projectPoint(second.a(), droppedAxis),
        projectPoint(second.b(), droppedAxis),
        projectPoint(second.c(), droppedAxis)};
    return !hasSeparatingEdge(projectedFirst, projectedSecond) &&
           !hasSeparatingEdge(projectedSecond, projectedFirst);
}

} // namespace

std::size_t
PolyCalculator::findIsolatedVertexIndex(const planeRelativeTriangle_t& curTriangle) {

    const int aVertOrientToNorm = -orientationSign(curTriangle.vertices[0].orient);
    const int bVertOrientToNorm = -orientationSign(curTriangle.vertices[1].orient);
    const int cVertOrientToNorm = -orientationSign(curTriangle.vertices[2].orient);

    std::size_t isolatedVertIdx = 0;
    if (aVertOrientToNorm != 0) {
        if (bVertOrientToNorm == aVertOrientToNorm)
            isolatedVertIdx = 2;
        else if (cVertOrientToNorm == aVertOrientToNorm)
            isolatedVertIdx = 1;
    } else if (bVertOrientToNorm != 0) {
        if (bVertOrientToNorm != cVertOrientToNorm)
            isolatedVertIdx = 1;
    } else {
        isolatedVertIdx = 2;
    }

    return isolatedVertIdx;
}

void PolyCalculator::canonicalizeTriangle(planeRelativeTriangle_t& curTriangle,
                                          planeRelativeTriangle_t& planeTriangle) {

    const std::size_t isolatedVertIdx = findIsolatedVertexIndex(curTriangle);

    std::rotate(curTriangle.vertices.begin(),
                curTriangle.vertices.begin() + isolatedVertIdx,
                curTriangle.vertices.end());

    const int firstSign = -orientationSign(curTriangle.vertices[0].orient);
    const int secondSign = -orientationSign(curTriangle.vertices[1].orient);

    if (firstSign < 0 || (firstSign == 0 && secondSign > 0)) {
        std::swap(planeTriangle.vertices[1], planeTriangle.vertices[2]);
        for (auto& vertex : curTriangle.vertices) {
            vertex.orient = -vertex.orient;
        }
    }
}

IntersectionData PolyCalculator::calculateIntersections(const Polygons& polys) {
    IntersectionData intersections;

    for (std::size_t first = 0; first < polys.size(); ++first) {
        const auto& polygon = polys[first];
        if (!polygon) {
            throw std::invalid_argument("Polygon must not be null");
        }
        if (polygon.get_deleter().kind != PolygonKind::Triangle) {
            throw std::invalid_argument("Only triangles are supported; other "
                                        "polygons are not supported yet");
        }
        const auto& triangle = static_cast<const triangle_t&>(*polygon);

        for (std::size_t second = 0; second < first; ++second) {
            const auto& other = static_cast<const triangle_t&>(*polys[second]);
            if (GuigueDevillers(triangle, other)) {
                intersections.emplace_back(second, first);
            }
        }
    }

    return intersections;
}

bool PolyCalculator::GuigueDevillers(const triangle_t& first,
                                     const triangle_t& second) {

    auto firstPos = getTrianglePosToPlaneOfTriangle(first, second);
    if (firstPos.relation == trianglePosToPlane_t::DOES_NOT_INTERSECT) {
        return false;
    }
    if (firstPos.relation == trianglePosToPlane_t::BELONGS) {
        return isTrianglesIntersect2D(first, second);
    }

    auto secondPos = getTrianglePosToPlaneOfTriangle(second, first);
    if (secondPos.relation == trianglePosToPlane_t::DOES_NOT_INTERSECT) {
        return false;
    }

    canonicalizeTriangle(firstPos, secondPos);
    canonicalizeTriangle(secondPos, firstPos);

    const auto& p1 = firstPos.vertices[0].vertex;
    const auto& q1 = firstPos.vertices[1].vertex;
    const auto& r1 = firstPos.vertices[2].vertex;

    const auto& p2 = secondPos.vertices[0].vertex;
    const auto& q2 = secondPos.vertices[1].vertex;
    const auto& r2 = secondPos.vertices[2].vertex;

    return orientationSign(point_t::orient3D(q1, p2, p1, q2)) >= 0 &&
           orientationSign(point_t::orient3D(p1, p2, r1, r2)) >= 0;
}

PolyCalculator::planeRelativeTriangle_t
PolyCalculator::getTrianglePosToPlaneOfTriangle(const triangle_t& curTriangle,
                                                const triangle_t& planeTriangle) {
    const double firstVertexOrient = point_t::orient3D(
        planeTriangle.a(), planeTriangle.b(), planeTriangle.c(), curTriangle.a());
    const double secondVertexOrient = point_t::orient3D(
        planeTriangle.a(), planeTriangle.b(), planeTriangle.c(), curTriangle.b());
    const double thirdVertexOrient = point_t::orient3D(
        planeTriangle.a(), planeTriangle.b(), planeTriangle.c(), curTriangle.c());

    planeRelativeTriangle_t result{{{{curTriangle.a(), firstVertexOrient},
                                     {curTriangle.b(), secondVertexOrient},
                                     {curTriangle.c(), thirdVertexOrient}}},
                                   trianglePosToPlane_t::DOES_NOT_INTERSECT};

    bool hasPositive = false;
    bool hasNegative = false;
    bool hasZero = false;

    for (double orientation :
         {firstVertexOrient, secondVertexOrient, thirdVertexOrient}) {
        if (orientationSign(orientation) > 0) {
            hasPositive = true;
        } else if (orientationSign(orientation) < 0) {
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
