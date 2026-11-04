#include "polyCalculator.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace triangles {

namespace {

using geometry::point_t;
using geometry::triangle_t;

enum class trianglePosToPlane_t { INTERSECTS, BELONGS, DOES_NOT_INTERSECT };

struct planeRelativeVertex_t {
    geometry::point_t<3> vertex;
    double orient;
};

struct planeRelativeTriangle_t {
    std::array<planeRelativeVertex_t, 3> vertices;
    trianglePosToPlane_t relation;
};

constexpr double ORIENTATION_EPSILON = 1e-9;

int orientationSign(double orientation) {
    if (orientation > ORIENTATION_EPSILON)
        return 1;
    if (orientation < -ORIENTATION_EPSILON)
        return -1;
    return 0;
}

bool triangleHasEdgeSeparatingFromOther(
    const triangle_t<2>& curTriangle,
    const triangle_t<2>& otherTriangle) {

    for (std::size_t edgeNum = 0; edgeNum < 3; ++edgeNum) {
        const auto& firstEdgeVert = curTriangle.point(edgeNum);              //TODO
        const auto& secondEdgeVert = curTriangle.point((edgeNum + 1) % 3);  // segment edge??
        const auto& freeVert = curTriangle.point((edgeNum + 2) % 3);

        const int insideSign = orientationSign(
            point_t<2>::orient2D(firstEdgeVert, secondEdgeVert, freeVert));
        // TODO pointToSegmentOrientFunc
        bool allOutside = true;
        for (const auto& point : otherTriangle.points()) {
            if (orientationSign(point_t<2>::orient2D(firstEdgeVert, secondEdgeVert, point)) != -insideSign ||
                insideSign == 0) {
                allOutside = false;
                break;
            }
        }
        if (allOutside)
            return true;
    }

    return false;
}

bool areTrianglesIntersectOnPlane(const triangle_t<3>& first, const triangle_t<3>& second) {
    const auto normal = first.unitNormal();

    const std::array<double, 3> normalComponents{
        std::abs(normal.x()), std::abs(normal.y()), std::abs(normal.z())};
    const std::size_t droppedAxis =
        std::max_element(normalComponents.begin(), normalComponents.end()) -
        normalComponents.begin();

    const triangle_t<2> projectedFirst{
        first.a().project(droppedAxis),
        first.b().project(droppedAxis),
        first.c().project(droppedAxis)};

    const triangle_t<2> projectedSecond{
        second.a().project(droppedAxis),
        second.b().project(droppedAxis),
        second.c().project(droppedAxis)};

    return !triangleHasEdgeSeparatingFromOther(projectedFirst, projectedSecond) &&
           !triangleHasEdgeSeparatingFromOther(projectedSecond, projectedFirst);
}

std::size_t
findIsolatedVertexIndex(const planeRelativeTriangle_t& curTriangle) {
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

void canonicalizeTriangle(planeRelativeTriangle_t& curTriangle,
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

planeRelativeTriangle_t
getTrianglePosToPlaneOfTriangle(const triangle_t<3>& curTriangle,
                                const triangle_t<3>& planeTriangle) {
    const double firstVertexOrient = point_t<3>::orient3D(
        planeTriangle.a(), planeTriangle.b(), planeTriangle.c(), curTriangle.a());
    const double secondVertexOrient = point_t<3>::orient3D(
        planeTriangle.a(), planeTriangle.b(), planeTriangle.c(), curTriangle.b());
    const double thirdVertexOrient = point_t<3>::orient3D(
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

bool doTrianglesIntersect(const triangle_t<3>& first, const triangle_t<3>& second) {
    auto firstPos = getTrianglePosToPlaneOfTriangle(first, second);
    if (firstPos.relation == trianglePosToPlane_t::DOES_NOT_INTERSECT) {
        return false;
    }
    if (firstPos.relation == trianglePosToPlane_t::BELONGS) {
        return areTrianglesIntersectOnPlane(first, second);
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

    return orientationSign(point_t<3>::orient3D(q1, p2, p1, q2)) >= 0 &&
           orientationSign(point_t<3>::orient3D(p1, p2, r1, r2)) >= 0;
}

} // namespace

IntersectionData PolyCalculator::calculateIntersections(const PolyWorld<3>& world) {
    IntersectionData intersections{world, {}, {}};

    const auto& polys = world.polys();
    std::vector<triangle_t<3>> triangles;
    triangles.reserve(polys.size());

    for (const auto& polygon : polys) {
        if (polygon.numOfPoints() != 3) {
            throw std::invalid_argument("Only triangles are supported; other "
                                        "polygons are not supported yet");
        }
        triangles.emplace_back(polygon.point(0), polygon.point(1), polygon.point(2));
    }

    std::vector<bool> intersects(triangles.size(), false);
    for (std::size_t first = 0; first < triangles.size(); ++first) {
        const auto& triangle = triangles[first];

        for (std::size_t second = 0; second < first; ++second) {
            const auto& other = triangles[second];
            if (doTrianglesIntersect(triangle, other)) {
                intersects[first] = true;
                intersects[second] = true;
            }
        }
    }

    for (std::size_t i = 0; i < polys.size(); ++i) {
        auto& collection = intersects[i] ? intersections.intersectingPolys
                                        : intersections.nonIntersectingPolys;
        collection.push_back(&polys[i]);
    }

    return intersections;
}

} // namespace triangles
