#include <algorithm>
#include <array>
#include <cstddef>
#include <ostream>
#include <string>

#include <gtest/gtest.h>

#include "polygonProcessor/polyWorldCalculator/intersections/intersection.hpp"

namespace {

using Coordinates = std::array<double, 3>;
using Vertices = std::array<Coordinates, 3>;
enum class IntersectionKind {
    POINT_POINT,
    POINT_SEGMENT,
    POINT_TRIANGLE,
    SEGMENT_SEGMENT,
    SEGMENT_TRIANGLE
};

constexpr Vertices point(Coordinates coordinates) {
    return {coordinates, coordinates, coordinates};
}

constexpr Vertices segment(Coordinates start, Coordinates end) {
    return {start, end, start};
}

constexpr Vertices BASE_TRIANGLE{{{0, 0, 0}, {4, 0, 0}, {0, 4, 0}}};
constexpr Vertices COLLINEAR_TRIANGLE{{{0, 0, 0}, {2, 0, 0}, {4, 0, 0}}};

TEST(RealTriangleForm, ClassifiesGeometryIndependentlyOfVertexOrder) {
    using triangles::RealTriangleForm;
    struct FormCase {
        Vertices vertices;
        RealTriangleForm expected;
    };
    const std::array cases{
        FormCase{point({1, 2, 3}), RealTriangleForm::POINT},
        FormCase{segment({0, 0, 0}, {4, 0, 0}), RealTriangleForm::SEGMENT},
        FormCase{COLLINEAR_TRIANGLE, RealTriangleForm::SEGMENT},
        FormCase{{{{1, 2, 3}, {3, 6, 9}, {5, 10, 15}}}, RealTriangleForm::SEGMENT},
        FormCase{{{{0, 0, 0}, {0.5e-10, 0, 0}, {1e-10, 0, 0}}},
                 RealTriangleForm::SEGMENT},
        FormCase{BASE_TRIANGLE, RealTriangleForm::TRIANGLE},
        FormCase{{{{0, 0, 0}, {1e-10, 0, 0}, {0, 1e-10, 0}}},
                 RealTriangleForm::TRIANGLE},
        FormCase{{{{0, 0, 0}, {1, 0, 0}, {2, 1e-10, 0}}},
                 RealTriangleForm::TRIANGLE}};

    for (std::size_t i = 0; i < cases.size(); ++i) {
        const auto& testCase = cases[i];
        std::array<std::size_t, 3> order{0, 1, 2};
        do {
            SCOPED_TRACE(testing::Message() << "case=" << i << " order=" << order[0]
                                            << order[1] << order[2]);
            const geometry::triangle_t<3> triangle(
                geometry::point_t<3>(testCase.vertices[order[0]]),
                geometry::point_t<3>(testCase.vertices[order[1]]),
                geometry::point_t<3>(testCase.vertices[order[2]]));
            EXPECT_EQ(triangles::determineRealTriangleForm(triangle),
                      testCase.expected);
        } while (std::next_permutation(order.begin(), order.end()));
    }
}

struct IntersectionCase {
    const char* name;
    IntersectionKind kind;
    Vertices first;
    Vertices second;
    bool expected;
};

std::ostream& operator<<(std::ostream& output, const IntersectionCase& testCase) {
    return output << testCase.name;
}

geometry::segment_t makeSegment(const geometry::triangle_t<3>& vertices) {
    const std::array lengths{vertices.lenAB(), vertices.lenBC(), vertices.lenCA()};
    const std::size_t first =
        std::max_element(lengths.begin(), lengths.end()) - lengths.begin();
    return geometry::segment_t(vertices.point(first),
                               vertices.point((first + 1) % 3));
}

bool intersect(IntersectionKind kind, const geometry::triangle_t<3>& first,
               const geometry::triangle_t<3>& second) {
    switch (kind) {
        case IntersectionKind::POINT_POINT:
            return triangles::doPointAndPointIntersect(first.a(), second.a());
        case IntersectionKind::POINT_SEGMENT:
            return triangles::doPointAndSegmentIntersect(first.a(),
                                                         makeSegment(second));
        case IntersectionKind::POINT_TRIANGLE:
            return triangles::doPointAndTriangleIntersect(first.a(), second);
        case IntersectionKind::SEGMENT_SEGMENT:
            return triangles::doSegmentAndSegmentIntersect(makeSegment(first),
                                                           makeSegment(second));
        case IntersectionKind::SEGMENT_TRIANGLE:
            return triangles::doSegmentAndTriangleIntersect(makeSegment(first),
                                                            second);
    }
    ADD_FAILURE() << "Unknown intersection kind";
    return false;
}

class TriangleFormIntersection : public testing::TestWithParam<IntersectionCase> {};

TEST_P(TriangleFormIntersection, IsIndependentOfVertexOrderAndProjectionAxis) {
    const auto& testCase = GetParam();
    constexpr std::array<std::array<std::size_t, 3>, 6> VERTEX_ORDERS{
        {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}}};

    for (std::size_t axis = 0; axis < 3; ++axis) {
        const auto transform = [axis](const Coordinates& coordinates) {
            return geometry::point_t<3>({coordinates[axis],
                                         coordinates[(axis + 1) % 3],
                                         coordinates[(axis + 2) % 3]});
        };
        for (const auto& firstOrder : VERTEX_ORDERS) {
            for (const auto& secondOrder : VERTEX_ORDERS) {
                SCOPED_TRACE(testing::Message()
                             << "axis=" << axis << " first=" << firstOrder[0]
                             << firstOrder[1] << firstOrder[2] << " second="
                             << secondOrder[0] << secondOrder[1] << secondOrder[2]);
                const geometry::triangle_t<3> first(
                    transform(testCase.first[firstOrder[0]]),
                    transform(testCase.first[firstOrder[1]]),
                    transform(testCase.first[firstOrder[2]]));
                const geometry::triangle_t<3> second(
                    transform(testCase.second[secondOrder[0]]),
                    transform(testCase.second[secondOrder[1]]),
                    transform(testCase.second[secondOrder[2]]));

                EXPECT_EQ(intersect(testCase.kind, first, second),
                          testCase.expected);
                if (testCase.kind == IntersectionKind::POINT_POINT ||
                    testCase.kind == IntersectionKind::SEGMENT_SEGMENT) {
                    EXPECT_EQ(intersect(testCase.kind, second, first),
                              testCase.expected);
                }
            }
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
    GeometricCases, TriangleFormIntersection,
    testing::Values(IntersectionCase{"IdenticalPoints",
                                     IntersectionKind::POINT_POINT,
                                     point({1, 2, 3}),
                                     point({1, 2, 3}),
                                     true},
                    IntersectionCase{"PointsAtDifferentDepths",
                                     IntersectionKind::POINT_POINT,
                                     point({1, 2, 3}),
                                     point({1, 2, 4}),
                                     false},
                    IntersectionCase{"PointsWithinTolerance",
                                     IntersectionKind::POINT_POINT,
                                     point({0, 0, 0}),
                                     point({0.5e-9, 0, 0}),
                                     true},
                    IntersectionCase{"PointsOutsideTolerance",
                                     IntersectionKind::POINT_POINT,
                                     point({0, 0, 0}),
                                     point({2e-9, 0, 0}),
                                     false},
                    IntersectionCase{"PointInsideSegment",
                                     IntersectionKind::POINT_SEGMENT,
                                     point({2, 0, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     true},
                    IntersectionCase{"PointAtSegmentEndpoint",
                                     IntersectionKind::POINT_SEGMENT,
                                     point({4, 0, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     true},
                    IntersectionCase{"PointOnSegmentExtension",
                                     IntersectionKind::POINT_SEGMENT,
                                     point({5, 0, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     false},
                    IntersectionCase{"PointOffSegmentLine",
                                     IntersectionKind::POINT_SEGMENT,
                                     point({2, 1, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     false},
                    IntersectionCase{"PointAboveSegment",
                                     IntersectionKind::POINT_SEGMENT,
                                     point({2, 0, 1}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     false},
                    IntersectionCase{"PointOnShortSegment",
                                     IntersectionKind::POINT_SEGMENT,
                                     point({0.5e-8, 0, 0}),
                                     segment({0, 0, 0}, {1e-8, 0, 0}),
                                     true},
                    IntersectionCase{"PointBeforeStartWithinTolerance",
                                     IntersectionKind::POINT_SEGMENT,
                                     point({-0.5e-9, 0, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     true},
                    IntersectionCase{"PointBeforeStartOutsideTolerance",
                                     IntersectionKind::POINT_SEGMENT,
                                     point({-2e-9, 0, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     false},
                    IntersectionCase{"PointPastEndWithinTolerance",
                                     IntersectionKind::POINT_SEGMENT,
                                     point({4 + 0.5e-9, 0, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     true},
                    IntersectionCase{"PointPastEndOutsideTolerance",
                                     IntersectionKind::POINT_SEGMENT,
                                     point({4 + 2e-9, 0, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     false},
                    IntersectionCase{"PointNearLineWithinTolerance",
                                     IntersectionKind::POINT_SEGMENT,
                                     point({2, 0.5e-9, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     true},
                    IntersectionCase{"PointNearLineOutsideTolerance",
                                     IntersectionKind::POINT_SEGMENT,
                                     point({2, 2e-9, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     false},
                    IntersectionCase{"PointOffShortSegment",
                                     IntersectionKind::POINT_SEGMENT,
                                     point({0.5e-8, 2e-9, 0}),
                                     segment({0, 0, 0}, {1e-8, 0, 0}),
                                     false},
                    IntersectionCase{"PointNearLongSegment",
                                     IntersectionKind::POINT_SEGMENT,
                                     point({5e5, 0.5e-9, 0}),
                                     segment({0, 0, 0}, {1e6, 0, 0}),
                                     true},
                    IntersectionCase{"PointInsideTriangle",
                                     IntersectionKind::POINT_TRIANGLE,
                                     point({1, 1, 0}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"PointOnTriangleEdge",
                                     IntersectionKind::POINT_TRIANGLE,
                                     point({2, 2, 0}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"PointAtTriangleVertex",
                                     IntersectionKind::POINT_TRIANGLE,
                                     point({4, 0, 0}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"PointOutsideTriangle",
                                     IntersectionKind::POINT_TRIANGLE,
                                     point({3, 3, 0}),
                                     BASE_TRIANGLE,
                                     false},
                    IntersectionCase{"PointAboveTriangle",
                                     IntersectionKind::POINT_TRIANGLE,
                                     point({1, 1, 1}),
                                     BASE_TRIANGLE,
                                     false},
                    IntersectionCase{"PointInsideObliqueTriangle",
                                     IntersectionKind::POINT_TRIANGLE,
                                     point({1, 1, 2}),
                                     {{{0, 0, 0}, {4, 0, 4}, {0, 4, 4}}},
                                     true},
                    IntersectionCase{"PointOutsideTinyTriangle",
                                     IntersectionKind::POINT_TRIANGLE,
                                     point({1, 1, 0}),
                                     {{{0, 0, 0}, {1e-5, 0, 0}, {0, 1e-5, 0}}},
                                     false},
                    IntersectionCase{"PointOnConvertedSegment",
                                     IntersectionKind::POINT_SEGMENT,
                                     point({3, 0, 0}),
                                     COLLINEAR_TRIANGLE,
                                     true},
                    IntersectionCase{"PointOutsideConvertedSegment",
                                     IntersectionKind::POINT_SEGMENT,
                                     point({5, 0, 0}),
                                     COLLINEAR_TRIANGLE,
                                     false},
                    IntersectionCase{"CrossingSegments",
                                     IntersectionKind::SEGMENT_SEGMENT,
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     segment({2, -1, 0}, {2, 1, 0}),
                                     true},
                    IntersectionCase{"SkewSegments",
                                     IntersectionKind::SEGMENT_SEGMENT,
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     segment({2, -1, 1}, {2, 1, 1}),
                                     false},
                    IntersectionCase{"CollinearOverlap",
                                     IntersectionKind::SEGMENT_SEGMENT,
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     segment({2, 0, 0}, {6, 0, 0}),
                                     true},
                    IntersectionCase{"CollinearContainment",
                                     IntersectionKind::SEGMENT_SEGMENT,
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     segment({1, 0, 0}, {3, 0, 0}),
                                     true},
                    IntersectionCase{"CollinearDisjointSegments",
                                     IntersectionKind::SEGMENT_SEGMENT,
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     segment({5, 0, 0}, {6, 0, 0}),
                                     false},
                    IntersectionCase{"SegmentEndpointContact",
                                     IntersectionKind::SEGMENT_SEGMENT,
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     segment({4, 0, 0}, {4, 2, 0}),
                                     true},
                    IntersectionCase{"ParallelDisjointSegments",
                                     IntersectionKind::SEGMENT_SEGMENT,
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     segment({0, 1, 0}, {4, 1, 0}),
                                     false},
                    IntersectionCase{"AlmostParallelCrossingSegments",
                                     IntersectionKind::SEGMENT_SEGMENT,
                                     segment({0, 0, 0}, {1e6, 0, 0}),
                                     segment({0, -1, 0}, {1e6, 1, 0}),
                                     true},
                    IntersectionCase{"AlmostParallelDisjointSegments",
                                     IntersectionKind::SEGMENT_SEGMENT,
                                     segment({0, 0, 0}, {1e6, 0, 0}),
                                     segment({0, 1, 0}, {1e6, 3, 0}),
                                     false},
                    IntersectionCase{"PointOnSegmentMidpoint",
                                     IntersectionKind::POINT_SEGMENT,
                                     point({2, 0, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     true},
                    IntersectionCase{"SegmentCrossesTriangle",
                                     IntersectionKind::SEGMENT_TRIANGLE,
                                     segment({1, 1, -1}, {1, 1, 1}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"SegmentCrossesPlaneOutsideTriangle",
                                     IntersectionKind::SEGMENT_TRIANGLE,
                                     segment({5, 5, -1}, {5, 5, 1}),
                                     BASE_TRIANGLE,
                                     false},
                    IntersectionCase{"SegmentEndpointOnTriangle",
                                     IntersectionKind::SEGMENT_TRIANGLE,
                                     segment({1, 1, 0}, {1, 1, 1}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"CoplanarSegmentCrossesTriangle",
                                     IntersectionKind::SEGMENT_TRIANGLE,
                                     segment({-1, 1, 0}, {5, 1, 0}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"CoplanarSegmentInsideTriangle",
                                     IntersectionKind::SEGMENT_TRIANGLE,
                                     segment({1, 1, 0}, {2, 1, 0}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"CoplanarSegmentOutsideTriangle",
                                     IntersectionKind::SEGMENT_TRIANGLE,
                                     segment({4, 4, 0}, {6, 4, 0}),
                                     BASE_TRIANGLE,
                                     false},
                    IntersectionCase{"SegmentOverlapsTriangleEdge",
                                     IntersectionKind::SEGMENT_TRIANGLE,
                                     segment({-1, 0, 0}, {5, 0, 0}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"SegmentTouchesTriangleVertex",
                                     IntersectionKind::SEGMENT_TRIANGLE,
                                     segment({-1, 1, 0}, {1, -1, 0}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"SegmentAboveTriangle",
                                     IntersectionKind::SEGMENT_TRIANGLE,
                                     segment({1, 1, 1}, {1, 1, 2}),
                                     BASE_TRIANGLE,
                                     false},
                    IntersectionCase{"SegmentParallelToTrianglePlane",
                                     IntersectionKind::SEGMENT_TRIANGLE,
                                     segment({-1, 1, 1}, {5, 1, 1}),
                                     BASE_TRIANGLE,
                                     false},
                    IntersectionCase{"PointInTriangleInterior",
                                     IntersectionKind::POINT_TRIANGLE,
                                     point({1, 1, 0}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"SegmentCrossesConvertedSegment",
                                     IntersectionKind::SEGMENT_SEGMENT,
                                     segment({3, -1, 0}, {3, 1, 0}),
                                     COLLINEAR_TRIANGLE,
                                     true},
                    IntersectionCase{"ConvertedSegmentAboveTriangle",
                                     IntersectionKind::SEGMENT_TRIANGLE,
                                     {{{1, 1, 1}, {1, 1, 2}, {1, 1, 3}}},
                                     BASE_TRIANGLE,
                                     false},
                    IntersectionCase{"ConvertedSegmentsCross",
                                     IntersectionKind::SEGMENT_SEGMENT,
                                     COLLINEAR_TRIANGLE,
                                     {{{3, -1, 0}, {3, 0, 0}, {3, 1, 0}}},
                                     true},
                    IntersectionCase{"ConvertedSegmentsDisjoint",
                                     IntersectionKind::SEGMENT_SEGMENT,
                                     COLLINEAR_TRIANGLE,
                                     {{{1, 1, 0}, {2, 1, 0}, {3, 1, 0}}},
                                     false},
                    IntersectionCase{"ObliqueSegmentsCross",
                                     IntersectionKind::SEGMENT_SEGMENT,
                                     segment({0, 0, 0}, {4, 4, 4}),
                                     segment({0, 4, 4}, {4, 0, 0}),
                                     true},
                    IntersectionCase{"ObliqueSegmentsSkew",
                                     IntersectionKind::SEGMENT_SEGMENT,
                                     segment({0, 0, 0}, {4, 4, 4}),
                                     segment({0, 4, 5}, {4, 0, 1}),
                                     false},
                    IntersectionCase{"NearlyCoplanarSegmentsCross",
                                     IntersectionKind::SEGMENT_SEGMENT,
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     segment({2, -1, 0.5e-9}, {2, 1, 0.5e-9}),
                                     true},
                    IntersectionCase{"SegmentCrossesTriangleWithShortEdge",
                                     IntersectionKind::SEGMENT_TRIANGLE,
                                     segment({-1, 2, 0}, {1, 2, 0}),
                                     {{{0, 0, 0}, {1e-10, 0, 0}, {0, 4, 0}}},
                                     true},
                    IntersectionCase{"PointOffLongSegment",
                                     IntersectionKind::POINT_SEGMENT,
                                     point({5e5, 1e-3, 0}),
                                     segment({0, 0, 0}, {1e6, 0, 0}),
                                     false}),
    [](const testing::TestParamInfo<IntersectionCase>& info) {
        return std::string(info.param.name);
    });

} // namespace
