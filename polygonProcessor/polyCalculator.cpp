
#include "polyCalculator.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace triangles {

namespace {

constexpr double orientationEpsilon = 1e-9;

int orientationSign(double orientation) {
    if (orientation > orientationEpsilon)
        return 1;
    if (orientation < -orientationEpsilon)
        return -1;
    return 0;
}

void canonicalizeTriangle(planeRelativeTriangle_t& triangle,
                          planeRelativeTriangle_t& other) {
    // orient3D(a,b,c,p) in geometry has the opposite sign to n.dot(p-a).
    const int a = -orientationSign(triangle.vertices[0].orient);
    const int b = -orientationSign(triangle.vertices[1].orient);
    const int c = -orientationSign(triangle.vertices[2].orient);

    // Put the isolated vertex first, preserving cyclic order. When there are
    // zeros, prefer the nonzero vertex, except for a lone vertex on the plane.
    std::size_t isolated = 0;
    if (a != 0) {
        if (b == a)
            isolated = 2;
        else if (c == a)
            isolated = 1;
    } else if (b != 0) {
        if (b != c)
            isolated = 1;
    } else {
        isolated = 2;
    }

    std::rotate(triangle.vertices.begin(),
                triangle.vertices.begin() + isolated,
                triangle.vertices.end());

    // A nonzero isolated vertex must be on the positive normal side.
    // For a lone zero, the other two vertices must be on the negative side.
    const int firstSign = -orientationSign(triangle.vertices[0].orient);
    const int secondSign = -orientationSign(triangle.vertices[1].orient);
    if (firstSign < 0 || (firstSign == 0 && secondSign > 0)) {
        std::swap(other.vertices[1], other.vertices[2]);
        for (auto& vertex : triangle.vertices) {
            vertex.orient = -vertex.orient;
        }
    }
}

struct Point2D {
    double x;
    double y;
};

Point2D projectPoint(const geometry::point_t& point, std::size_t droppedAxis) {
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

bool isTrianglesIntersect2D(const geometry::triangle_t& first,
                            const geometry::triangle_t& second) {
    const auto normal = geometry::vector_t(first.a(), first.b())
                            .cross(geometry::vector_t(first.a(), first.c()));
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

void canonicalizePlaneRelativeTriangles(planeRelativeTriangle_t& first,
                                        planeRelativeTriangle_t& second) {
    assert(first.relation == trianglePosToPlane_t::INTERSECTS);
    assert(second.relation == trianglePosToPlane_t::INTERSECTS);
    canonicalizeTriangle(first, second);
    canonicalizeTriangle(second, first);
}

IntersectionData PolyCalculator::calculateIntersections(
    const std::vector<geometry::polygon_t>& polys) {
    for (const auto& poly : polys) {
        if (poly.numOfPoints() != 3) {
            throw std::invalid_argument("Only triangles are supported; other "
                                        "polygons are not supported yet");
        }
    }

    std::vector<geometry::triangle_t> triangles;
    triangles.reserve(polys.size());
    for (const auto& poly : polys) {
        triangles.emplace_back(poly.point(0), poly.point(1), poly.point(2));
    }

    IntersectionData intersections;
    for (std::size_t first = 0; first < polys.size(); ++first) {
        for (std::size_t second = first + 1; second < polys.size(); ++second) {
            if (GuigueDevillers(triangles[first], triangles[second])) {
                intersections.emplace_back(polys[first], polys[second]);
            }
        }
    }
    return intersections;
}

bool PolyCalculator::GuigueDevillers(const geometry::triangle_t& first,
                                     const geometry::triangle_t& second) {
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
    if (secondPos.relation == trianglePosToPlane_t::BELONGS) {
        return isTrianglesIntersect2D(first, second);
    }

    canonicalizePlaneRelativeTriangles(firstPos, secondPos);
    const auto& p1 = firstPos.vertices[0].vertex;
    const auto& q1 = firstPos.vertices[1].vertex;
    const auto& r1 = firstPos.vertices[2].vertex;
    const auto& p2 = secondPos.vertices[0].vertex;
    const auto& q2 = secondPos.vertices[1].vertex;
    const auto& r2 = secondPos.vertices[2].vertex;

    // Guigue–Devillers: the two canonical plane sections must overlap.
    // No intersection points or divisions are needed.
    return orientationSign(geometry::point_t::orient3D(q1, p2, p1, q2)) >= 0 &&
           orientationSign(geometry::point_t::orient3D(p1, p2, r1, r2)) >= 0;
}

planeRelativeTriangle_t
getTrianglePosToPlaneOfTriangle(const geometry::triangle_t& curTriangle,
                                const geometry::triangle_t& planeTriangle) {
    const double firstVertexOrient = geometry::point_t::orient3D(
        planeTriangle.a(), planeTriangle.b(), planeTriangle.c(), curTriangle.a());
    const double secondVertexOrient = geometry::point_t::orient3D(
        planeTriangle.a(), planeTriangle.b(), planeTriangle.c(), curTriangle.b());
    const double thirdVertexOrient = geometry::point_t::orient3D(
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
