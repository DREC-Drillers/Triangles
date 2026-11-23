#include <array>
#include <cstddef>
#include <ostream>
#include <string>

#include <gtest/gtest.h>

#include "polygonProcessor/polyWorldCalculator/intersections/intersection.hpp"

namespace {

using Coordinates = std::array<double, 3>;
using Vertices = std::array<Coordinates, 3>;

constexpr Vertices point(Coordinates coordinates) {
    return {coordinates, coordinates, coordinates};
}

constexpr Vertices segment(Coordinates start, Coordinates end) {
    return {start, end, start};
}

constexpr Vertices BASE_TRIANGLE{{{0, 0, 0}, {4, 0, 0}, {0, 4, 0}}};
constexpr Vertices COLLINEAR_TRIANGLE{{{0, 0, 0}, {2, 0, 0}, {4, 0, 0}}};

struct IntersectionCase {
    const char* name;
    Vertices first;
    Vertices second;
    bool expected;
};

std::ostream& operator<<(std::ostream& output, const IntersectionCase& testCase) {
    return output << testCase.name;
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

                EXPECT_EQ(triangles::doIntersect(first, second), testCase.expected);
                EXPECT_EQ(triangles::doIntersect(second, first), testCase.expected);
            }
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
    GeometricCases, TriangleFormIntersection,
    testing::Values(IntersectionCase{"IdenticalPoints",
                                     point({1, 2, 3}),
                                     point({1, 2, 3}),
                                     true},
                    IntersectionCase{"PointsAtDifferentDepths",
                                     point({1, 2, 3}),
                                     point({1, 2, 4}),
                                     false},
                    IntersectionCase{"PointsWithinTolerance",
                                     point({0, 0, 0}),
                                     point({0.5e-9, 0, 0}),
                                     true},
                    IntersectionCase{"PointsOutsideTolerance",
                                     point({0, 0, 0}),
                                     point({2e-9, 0, 0}),
                                     false},
                    IntersectionCase{"PointInsideSegment",
                                     point({2, 0, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     true},
                    IntersectionCase{"PointAtSegmentEndpoint",
                                     point({4, 0, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     true},
                    IntersectionCase{"PointOnSegmentExtension",
                                     point({5, 0, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     false},
                    IntersectionCase{"PointOffSegmentLine",
                                     point({2, 1, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     false},
                    IntersectionCase{"PointAboveSegment",
                                     point({2, 0, 1}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     false},
                    IntersectionCase{"PointOnShortSegment",
                                     point({0.5e-8, 0, 0}),
                                     segment({0, 0, 0}, {1e-8, 0, 0}),
                                     true},
                    IntersectionCase{"PointNearVeryShortCollinearTriangle",
                                     point({1.05e-9, 0, 0}),
                                     {{{0, 0, 0}, {0.5e-10, 0, 0}, {1e-10, 0, 0}}},
                                     true},
                    IntersectionCase{"PointBeforeStartWithinTolerance",
                                     point({-0.5e-9, 0, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     true},
                    IntersectionCase{"PointBeforeStartOutsideTolerance",
                                     point({-2e-9, 0, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     false},
                    IntersectionCase{"PointPastEndWithinTolerance",
                                     point({4 + 0.5e-9, 0, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     true},
                    IntersectionCase{"PointPastEndOutsideTolerance",
                                     point({4 + 2e-9, 0, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     false},
                    IntersectionCase{"PointNearLineWithinTolerance",
                                     point({2, 0.5e-9, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     true},
                    IntersectionCase{"PointNearLineOutsideTolerance",
                                     point({2, 2e-9, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     false},
                    IntersectionCase{"PointOffShortSegment",
                                     point({0.5e-8, 2e-9, 0}),
                                     segment({0, 0, 0}, {1e-8, 0, 0}),
                                     false},
                    IntersectionCase{"PointNearLongSegment",
                                     point({5e5, 0.5e-9, 0}),
                                     segment({0, 0, 0}, {1e6, 0, 0}),
                                     true},
                    IntersectionCase{"PointInsideTriangle",
                                     point({1, 1, 0}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"PointOnTriangleEdge",
                                     point({2, 2, 0}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"PointAtTriangleVertex",
                                     point({4, 0, 0}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"PointOutsideTriangle",
                                     point({3, 3, 0}),
                                     BASE_TRIANGLE,
                                     false},
                    IntersectionCase{"PointAboveTriangle",
                                     point({1, 1, 1}),
                                     BASE_TRIANGLE,
                                     false},
                    IntersectionCase{"PointInsideObliqueTriangle",
                                     point({1, 1, 2}),
                                     {{{0, 0, 0}, {4, 0, 4}, {0, 4, 4}}},
                                     true},
                    IntersectionCase{"PointOutsideTinyTriangle",
                                     point({1, 1, 0}),
                                     {{{0, 0, 0}, {1e-5, 0, 0}, {0, 1e-5, 0}}},
                                     false},
                    IntersectionCase{"PointNearCornerOfTinyTriangle",
                                     point({-0.8e-9, -0.8e-9, 0}),
                                     {{{0, 0, 0}, {1e-10, 0, 0}, {0, 1e-10, 0}}},
                                     true},
                    IntersectionCase{"PointNearCornerOfThinTriangle",
                                     point({1, -0.98e-9, 0}),
                                     {{{0, 0, 0}, {1, 0, 0}, {2, 1e-10, 0}}},
                                     true},
                    IntersectionCase{"PointOnConvertedSegment",
                                     point({3, 0, 0}),
                                     COLLINEAR_TRIANGLE,
                                     true},
                    IntersectionCase{"PointOutsideConvertedSegment",
                                     point({5, 0, 0}),
                                     COLLINEAR_TRIANGLE,
                                     false},
                    IntersectionCase{"PointOnObliqueCollinearTriangle",
                                     point({2.5, 3.5, 4.5}),
                                     {{{1, 2, 3}, {3, 4, 5}, {5, 6, 7}}},
                                     true},
                    IntersectionCase{"CrossingSegments",
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     segment({2, -1, 0}, {2, 1, 0}),
                                     true},
                    IntersectionCase{"SkewSegments",
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     segment({2, -1, 1}, {2, 1, 1}),
                                     false},
                    IntersectionCase{"CollinearOverlap",
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     segment({2, 0, 0}, {6, 0, 0}),
                                     true},
                    IntersectionCase{"CollinearContainment",
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     segment({1, 0, 0}, {3, 0, 0}),
                                     true},
                    IntersectionCase{"CollinearDisjointSegments",
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     segment({5, 0, 0}, {6, 0, 0}),
                                     false},
                    IntersectionCase{"SegmentEndpointContact",
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     segment({4, 0, 0}, {4, 2, 0}),
                                     true},
                    IntersectionCase{"ParallelDisjointSegments",
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     segment({0, 1, 0}, {4, 1, 0}),
                                     false},
                    IntersectionCase{"AlmostParallelCrossingSegments",
                                     segment({0, 0, 0}, {1e6, 0, 0}),
                                     segment({0, -1, 0}, {1e6, 1, 0}),
                                     true},
                    IntersectionCase{"AlmostParallelDisjointSegments",
                                     segment({0, 0, 0}, {1e6, 0, 0}),
                                     segment({0, 1, 0}, {1e6, 3, 0}),
                                     false},
                    IntersectionCase{"PointOnSegmentMidpoint",
                                     point({2, 0, 0}),
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     true},
                    IntersectionCase{"SegmentCrossesTriangle",
                                     segment({1, 1, -1}, {1, 1, 1}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"SegmentCrossesPlaneOutsideTriangle",
                                     segment({5, 5, -1}, {5, 5, 1}),
                                     BASE_TRIANGLE,
                                     false},
                    IntersectionCase{"SegmentEndpointOnTriangle",
                                     segment({1, 1, 0}, {1, 1, 1}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"CoplanarSegmentCrossesTriangle",
                                     segment({-1, 1, 0}, {5, 1, 0}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"CoplanarSegmentInsideTriangle",
                                     segment({1, 1, 0}, {2, 1, 0}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"CoplanarSegmentOutsideTriangle",
                                     segment({4, 4, 0}, {6, 4, 0}),
                                     BASE_TRIANGLE,
                                     false},
                    IntersectionCase{"SegmentOverlapsTriangleEdge",
                                     segment({-1, 0, 0}, {5, 0, 0}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"SegmentTouchesTriangleVertex",
                                     segment({-1, 1, 0}, {1, -1, 0}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"SegmentAboveTriangle",
                                     segment({1, 1, 1}, {1, 1, 2}),
                                     BASE_TRIANGLE,
                                     false},
                    IntersectionCase{"SegmentParallelToTrianglePlane",
                                     segment({-1, 1, 1}, {5, 1, 1}),
                                     BASE_TRIANGLE,
                                     false},
                    IntersectionCase{"PointInTriangleInterior",
                                     point({1, 1, 0}),
                                     BASE_TRIANGLE,
                                     true},
                    IntersectionCase{"SegmentCrossesConvertedSegment",
                                     segment({3, -1, 0}, {3, 1, 0}),
                                     COLLINEAR_TRIANGLE,
                                     true},
                    IntersectionCase{"ConvertedSegmentAboveTriangle",
                                     {{{1, 1, 1}, {1, 1, 2}, {1, 1, 3}}},
                                     BASE_TRIANGLE,
                                     false},
                    IntersectionCase{"ConvertedSegmentsCross",
                                     COLLINEAR_TRIANGLE,
                                     {{{3, -1, 0}, {3, 0, 0}, {3, 1, 0}}},
                                     true},
                    IntersectionCase{"ConvertedSegmentsDisjoint",
                                     COLLINEAR_TRIANGLE,
                                     {{{1, 1, 0}, {2, 1, 0}, {3, 1, 0}}},
                                     false},
                    IntersectionCase{"ObliqueSegmentsCross",
                                     segment({0, 0, 0}, {4, 4, 4}),
                                     segment({0, 4, 4}, {4, 0, 0}),
                                     true},
                    IntersectionCase{"ObliqueSegmentsSkew",
                                     segment({0, 0, 0}, {4, 4, 4}),
                                     segment({0, 4, 5}, {4, 0, 1}),
                                     false},
                    IntersectionCase{"NearlyCoplanarSegmentsCross",
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     segment({2, -1, 0.5e-9}, {2, 1, 0.5e-9}),
                                     true},
                    IntersectionCase{"SegmentsAtCoplanarityToleranceCross",
                                     segment({0, 0, 0}, {4, 0, 0}),
                                     segment({2, -1, 1e-9}, {2, 1, 1e-9}),
                                     true},
                    IntersectionCase{"SegmentCrossesTriangleWithShortEdge",
                                     segment({-1, 2, 0}, {1, 2, 0}),
                                     {{{0, 0, 0}, {1e-10, 0, 0}, {0, 4, 0}}},
                                     true},
                    IntersectionCase{"PointOffLongSegment",
                                     point({5e5, 1e-3, 0}),
                                     segment({0, 0, 0}, {1e6, 0, 0}),
                                     false}),
    [](const testing::TestParamInfo<IntersectionCase>& info) {
        return std::string(info.param.name);
    });

} // namespace
