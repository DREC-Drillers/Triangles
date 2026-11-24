#include "triangleIntersection.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <utility>

namespace triangles {

namespace {

using geometry::point_t;
using geometry::triangle_t;

enum class TrianglePosToPlane { INTERSECTS, BELONGS, DOES_NOT_INTERSECT };

struct PlaneRelativeVertex {
    geometry::point_t<3> vertex;
    double orient;
};

struct PlaneRelativeTriangle {
    std::array<PlaneRelativeVertex, 3> vertices;
    TrianglePosToPlane relation;
};

constexpr double ORIENTATION_EPSILON = 1e-9;

int orientationSign(double orientation) {
    if (orientation > ORIENTATION_EPSILON)
        return 1;
    if (orientation < -ORIENTATION_EPSILON)
        return -1;
    return 0;
}

bool triangleHasEdgeSeparatingFromOther(const triangle_t<2>& curTriangle,
                                        const triangle_t<2>& otherTriangle) {
    for (std::size_t edgeNum = 0; edgeNum < 3; ++edgeNum) {
        const auto& firstEdgeVert = curTriangle.point(edgeNum);
        const auto& secondEdgeVert = curTriangle.point((edgeNum + 1) % 3);
        const auto& freeVert = curTriangle.point((edgeNum + 2) % 3);

        const int insideSign =
            point_t<2>::orient2D(firstEdgeVert, secondEdgeVert, freeVert) < 0 ? -1
                                                                              : 1;
        const double edgeLength = firstEdgeVert.distance(secondEdgeVert);
        bool allOutside = true;
        for (const auto& point : otherTriangle.points()) {
            if (orientationSign(
                    point_t<2>::orient2D(firstEdgeVert, secondEdgeVert, point) /
                    edgeLength) != -insideSign) {
                allOutside = false;
                break;
            }
        }
        if (allOutside)
            return true;
    }

    return false;
}

bool areTrianglesIntersectOnPlane(const triangle_t<3>& first,
                                  const triangle_t<3>& second) {
    const auto normal = first.unitNormal();

    const std::array<double, 3> normalComponents{
        std::abs(normal.x()), std::abs(normal.y()), std::abs(normal.z())};
    const std::size_t droppedAxis =
        std::max_element(normalComponents.begin(), normalComponents.end()) -
        normalComponents.begin();

    const triangle_t<2> projectedFirst{first.a().project(droppedAxis),
                                       first.b().project(droppedAxis),
                                       first.c().project(droppedAxis)};

    const triangle_t<2> projectedSecond{second.a().project(droppedAxis),
                                        second.b().project(droppedAxis),
                                        second.c().project(droppedAxis)};

    return !triangleHasEdgeSeparatingFromOther(projectedFirst, projectedSecond) &&
           !triangleHasEdgeSeparatingFromOther(projectedSecond, projectedFirst);
}

std::size_t findIsolatedVertexIndex(const PlaneRelativeTriangle& curTriangle) {
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

void canonicalizeTriangle(PlaneRelativeTriangle& curTriangle,
                          PlaneRelativeTriangle& planeTriangle) {
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

PlaneRelativeTriangle
getTrianglePosToPlaneOfTriangle(const triangle_t<3>& curTriangle,
                                const triangle_t<3>& planeTriangle) {
    const double firstVertexOrient = curTriangle.a().signedDistanceToPlane(
        planeTriangle.a(), planeTriangle.b(), planeTriangle.c());
    const double secondVertexOrient = curTriangle.b().signedDistanceToPlane(
        planeTriangle.a(), planeTriangle.b(), planeTriangle.c());
    const double thirdVertexOrient = curTriangle.c().signedDistanceToPlane(
        planeTriangle.a(), planeTriangle.b(), planeTriangle.c());

    PlaneRelativeTriangle result{{{{curTriangle.a(), firstVertexOrient},
                                   {curTriangle.b(), secondVertexOrient},
                                   {curTriangle.c(), thirdVertexOrient}}},
                                 TrianglePosToPlane::DOES_NOT_INTERSECT};

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
        result.relation = TrianglePosToPlane::BELONGS;
    } else if (hasZero || (hasPositive && hasNegative)) {
        result.relation = TrianglePosToPlane::INTERSECTS;
    }
    return result;
}

} // namespace

bool doTrianglesIntersect(const triangle_t<3>& first, const triangle_t<3>& second) {
    auto firstPos = getTrianglePosToPlaneOfTriangle(first, second);
    if (firstPos.relation == TrianglePosToPlane::DOES_NOT_INTERSECT) {
        return false;
    }
    auto secondPos = getTrianglePosToPlaneOfTriangle(second, first);
    if (secondPos.relation == TrianglePosToPlane::DOES_NOT_INTERSECT) {
        return false;
    }
    if (firstPos.relation == TrianglePosToPlane::BELONGS &&
        secondPos.relation == TrianglePosToPlane::BELONGS) {
        return areTrianglesIntersectOnPlane(first, second);
    }

    canonicalizeTriangle(firstPos, secondPos);
    canonicalizeTriangle(secondPos, firstPos);

    const auto& p1 = firstPos.vertices[0].vertex;
    const auto& q1 = firstPos.vertices[1].vertex;
    const auto& r1 = firstPos.vertices[2].vertex;

    const auto& p2 = secondPos.vertices[0].vertex;
    const auto& q2 = secondPos.vertices[1].vertex;
    const auto& r2 = secondPos.vertices[2].vertex;

    return orientationSign(q2.signedDistanceToPlane(q1, p2, p1)) >= 0 &&
           orientationSign(r2.signedDistanceToPlane(p1, p2, r1)) >= 0;
}

} // namespace triangles
