#include "intersection.hpp"

#include <cmath>
#include <cstddef>

namespace triangles {

namespace {

using geometry::point_t;
using geometry::segment_t;
using geometry::triangle_t;
using geometry::vector_t;

constexpr double DISTANCE_EPSILON = 1e-9;

} // namespace

RealTriangleForm determineRealTriangleForm(const triangle_t<3>& triangle) {
    if (triangle.a() == triangle.b() && triangle.b() == triangle.c()) {
        return RealTriangleForm::POINT;
    }
    return triangle.area() == 0.0 ? RealTriangleForm::SEGMENT
                                  : RealTriangleForm::TRIANGLE;
}

bool pointLiesInTrianglePlain(const point_t<3>& point, const triangle_t& triangle) {
    const auto normal = triangle.unitNormal();
    vector_t pointDir(triangle.a(), point);

    const auto pointDistanceToPlane = pointDir.dot(normal);
    return std::abs(pointDistance) < DISTANCE_EPSILON;
}

bool doPointAndPointIntersect(const point_t<3>& firstPoint,
                              const point_t<3>& secondPoint) {
    return firstPoint.isApprox(secondPoint);
}

bool doPointAndSegmentIntersect(const point_t<3>& point, const segment_t& segment) {
    const auto direction = segment.direction();
    const vector_t pointDir(segment.a(), point);

    return direction.cross(pointDir).length() <= DISTANCE_EPSILON * direction.length() &&
           direction.dot(pointDir) >= 0 &&
           pointDir.length() <= direction.length();
}

bool doPointAndTriangleIntersect(const point_t<3>& point,
                                 const triangle_t<3>& triangle) {

    if (!pointLiesInTrianglePlain(point, triangle)) {
        return false;
    }

    for (std::size_t i = 0; i < 3; ++i) {
        const auto& firstVert = triangle.point(i);
        const auto& secondVert = triangle.point((i + 1) % 3);

        const vector_t edge(firstVert, secondVert);

        if ((edge.cross(vector_t(firstVert, point))).dot(normal) / edge.length() <
            -DISTANCE_EPSILON) {
            return false;
        }
    }
    return true;
}

bool doSegmentAndSegmentIntersect(const segment_t& first, const segment_t& second) {
    if (doPointAndSegmentIntersect(first.a(), second) ||
        doPointAndSegmentIntersect(first.b(), second) ||
        doPointAndSegmentIntersect(second.a(), first) ||
        doPointAndSegmentIntersect(second.b(), first)) {
        return true;
    }

    const auto normal = first.direction().cross(second.direction());
    if (normal.length() == 0.0) {
        return false;
    }
    const auto unitNormal = normal.normalized();

    if (std::abs(vector_t(first.a(), second.a()).dot(unitNormal)) >=
            DISTANCE_EPSILON ||
        std::abs(vector_t(first.a(), second.b()).dot(unitNormal)) >=
            DISTANCE_EPSILON) {
        return false;
    }

    return first.isIntersectsCoplanar(second, unitNormal);
}

bool doSegmentIntersectTrianglePlain(const segment_t& segment, const triangle_t<3>& triangle) {
    const auto normal = triangle.unitNormal();

    const double startDistance = vector_t(triangle.a(), segment.a()).dot(normal);
    const double endDistance = vector_t(triangle.a(), segment.b()).dot(normal);

    if ((startDistance > 0.0 && endDistance > 0.0) ||
        (startDistance < 0.0 && endDistance < 0.0)) {
        return false;
    }

    return true;
}

point_t<3> getSegAndTrianglePlaneIntersectPoint(const segment_t& segment,
                                                const triangle_t<3>& triangle) {

    const double startDistance = vector_t(triangle.a(), segment.a()).dot(normal);
    const double endDistance = vector_t(triangle.a(), segment.b()).dot(normal);

    const auto pointDir =
        segment.direction() * (startDistance / (startDistance - endDistance));

    return point_t<3>{segment.a().x() + pointDir.x(),
                      segment.a().y() + pointDir.y(),
                      segment.a().z() + pointDir.z()}
}

bool doSegmentAndTriangleIntersect(const segment_t& segment,
                                   const triangle_t<3>& triangle) {
    if (doPointAndTriangleIntersect(segment.a(), triangle) ||
        doPointAndTriangleIntersect(segment.b(), triangle)) {
        return true;
    }

    if (pointLiesInTrianglePlain(segment.a(), triangle) &&
        pointLiesInTrianglePlain(segment.b(), triangle)) {
        for (std::size_t i = 0; i < 3; ++i) {
            const segment_t edge(triangle.point(i), triangle.point((i + 1) % 3));
            if (doSegmentAndSegmentIntersect(segment, edge)) {
                return true;
            }
        }

        return false;
    }

    if (!doSegmentIntersectTrianglePlain(segment, triangle)) {
        return false;
    }

    const point_t<3> intersectPoint = getSegAndTrianglePlaneIntersectPoint (segment, triangle);
    return doPointAndTriangleIntersect(intersectPoint, triangle);
}

} // namespace triangles
