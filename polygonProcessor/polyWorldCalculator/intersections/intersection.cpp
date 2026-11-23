#include "intersection.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <variant>

#include "triangleIntersection.hpp"

namespace triangles {

using geometry::point_t;
using geometry::segment_t;
using geometry::triangle_t;
using geometry::vector_t;

namespace {

constexpr double DISTANCE_EPSILON = 1e-9;

using TriangleForm = std::variant<point_t<3>, segment_t,
                                  std::reference_wrapper<const triangle_t<3>>>;

TriangleForm toRealForm(const triangle_t<3>& triangle) {
    switch (determineRealTriangleForm(triangle)) {
        case RealTriangleForm::POINT:
            return triangle.a();
        case RealTriangleForm::SEGMENT:
            return degenerateTriangleToSegment(triangle);
        case RealTriangleForm::TRIANGLE:
            return std::cref(triangle);
    }
    throw std::logic_error("Unknown triangle form");
}

struct Intersect {
    bool operator()(const point_t<3>& first, const point_t<3>& second) const {
        return doPointAndPointIntersect(first, second);
    }

    bool operator()(const point_t<3>& point, const segment_t& segment) const {
        return doPointAndSegmentIntersect(point, segment);
    }

    bool operator()(const point_t<3>& point, const triangle_t<3>& triangle) const {
        return doPointAndTriangleIntersect(point, triangle);
    }

    bool operator()(const segment_t& first, const segment_t& second) const {
        return doSegmentAndSegmentIntersect(first, second);
    }

    bool operator()(const segment_t& segment, const triangle_t<3>& triangle) const {
        return doSegmentAndTriangleIntersect(segment, triangle);
    }

    bool operator()(const triangle_t<3>& first, const triangle_t<3>& second) const {
        return doTrianglesIntersect(first, second);
    }

    bool operator()(const segment_t& segment, const point_t<3>& point) const {
        return (*this)(point, segment);
    }

    bool operator()(const triangle_t<3>& triangle, const point_t<3>& point) const {
        return (*this)(point, triangle);
    }

    bool operator()(const triangle_t<3>& triangle, const segment_t& segment) const {
        return (*this)(segment, triangle);
    }
};

} // namespace

bool doIntersect(const triangle_t<3>& first, const triangle_t<3>& second) {
    return std::visit(Intersect{}, toRealForm(first), toRealForm(second));
}

RealTriangleForm determineRealTriangleForm(const triangle_t<3>& triangle) {
    if (triangle.a() == triangle.b() && triangle.b() == triangle.c()) {
        return RealTriangleForm::POINT;
    }
    return triangle.area() == 0.0 ? RealTriangleForm::SEGMENT
                                  : RealTriangleForm::TRIANGLE;
}

segment_t degenerateTriangleToSegment(const triangle_t<3>& triangle) { //FIXME geometry
    const std::array lengths{triangle.lenAB(), triangle.lenBC(), triangle.lenCA()};
    const std::size_t first =
        std::max_element(lengths.begin(), lengths.end()) - lengths.begin();
    return segment_t(triangle.point(first), triangle.point((first + 1) % 3));
}

bool pointLiesInTrianglePlain(const point_t<3>& point,
                              const triangle_t<3>& triangle) {
    const auto normal = triangle.unitNormal();
    vector_t pointDir(triangle.a(), point);

    const auto pointDistanceToPlane = pointDir.dot(normal);
    return std::abs(pointDistanceToPlane) <= DISTANCE_EPSILON;
}

bool doPointAndPointIntersect(const point_t<3>& firstPoint,
                              const point_t<3>& secondPoint) {
    return firstPoint.isApprox(secondPoint);
}

bool doPointAndSegmentIntersect(const point_t<3>& point, const segment_t& segment) {
    const auto direction = segment.direction();
    const vector_t offset(segment.a(), point);
    const auto unitDirection = direction.normalized();

    const double projection = offset.dot(unitDirection);

    return unitDirection.cross(offset).length() <= DISTANCE_EPSILON &&
           projection >= -DISTANCE_EPSILON &&
           projection <= segment.length() + DISTANCE_EPSILON;
}

bool doPointAndTriangleIntersect(const point_t<3>& point,
                                 const triangle_t<3>& triangle) {
    if (!pointLiesInTrianglePlain(point, triangle)) {
        return false;
    }

    const auto normal = triangle.unitNormal();
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
    if (std::abs(vector_t(first.a(), second.a()).dot(unitNormal)) >
            DISTANCE_EPSILON ||
        std::abs(vector_t(first.a(), second.b()).dot(unitNormal)) >
            DISTANCE_EPSILON) {
        return false;
    }
    return first.isIntersectsCoplanar(second, unitNormal);
}

bool doSegmentIntersectTrianglePlain(const segment_t& segment,
                                     const triangle_t<3>& triangle) {
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
    const auto normal = triangle.unitNormal();
    const double startDistance = vector_t(triangle.a(), segment.a()).dot(normal);
    const double endDistance = vector_t(triangle.a(), segment.b()).dot(normal);

    const auto pointDir =
        segment.direction() * (startDistance / (startDistance - endDistance));

    return point_t<3>({segment.a().x() + pointDir.x(),
                       segment.a().y() + pointDir.y(),
                       segment.a().z() + pointDir.z()});
}

bool doSegmentAndTriangleIntersect(const segment_t& segment,
                                   const triangle_t<3>& triangle) {
    if (doPointAndTriangleIntersect(segment.a(), triangle) ||
        doPointAndTriangleIntersect(segment.b(), triangle)) {
        return true;
    }
    for (std::size_t i = 0; i < 3; ++i) {
        if (doSegmentAndSegmentIntersect(
                segment,
                segment_t(triangle.point(i), triangle.point((i + 1) % 3)))) {
            return true;
        }
    }

    if (pointLiesInTrianglePlain(segment.a(), triangle) &&
        pointLiesInTrianglePlain(segment.b(), triangle)) {
        return false;
    }

    if (!doSegmentIntersectTrianglePlain(segment, triangle)) {
        return false;
    }

    const auto intersection =
        getSegAndTrianglePlaneIntersectPoint(segment, triangle);
    return doPointAndTriangleIntersect(intersection, triangle);
}

} // namespace triangles
