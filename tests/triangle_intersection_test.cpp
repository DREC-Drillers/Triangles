#include <array>
#include <ostream>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include "polygonProcessor/initPolysData.hpp"
#include "polygonProcessor/polyWorldCalculator/intersections/triangleIntersection.hpp"

namespace {

using triangles::InitPolysData;

template <typename Point>
concept HasProjection = requires(const Point& point) {
    point.project(0);
};

static_assert(!HasProjection<geometry::point_t<1>>);
static_assert(HasProjection<geometry::point_t<2>>);

TEST(PointProjection, PreservesRemainingCoordinatesInOrder) {
    const geometry::point_t<3> point(std::array<double, 3>{1, 2, 3});
    EXPECT_EQ(point.project(0), (geometry::point_t<2>(std::array<double, 2>{2, 3})));
    EXPECT_EQ(point.project(1), (geometry::point_t<2>(std::array<double, 2>{1, 3})));
    EXPECT_EQ(point.project(2), (geometry::point_t<2>(std::array<double, 2>{1, 2})));
    EXPECT_DOUBLE_EQ(point.x(), 1);
    EXPECT_DOUBLE_EQ(point.y(), 2);
    EXPECT_DOUBLE_EQ(point.z(), 3);
}

TEST(PointProjection, SupportsOtherDimensionsAndRejectsInvalidAxis) {
    const geometry::point_t<2> plane(std::array<double, 2>{4, 5});
    EXPECT_DOUBLE_EQ(plane.project(0).x(), 5);
    EXPECT_DOUBLE_EQ(plane.project(1).x(), 4);
    EXPECT_THROW(plane.project(2), std::out_of_range);

    const geometry::point_t<4> point(std::array<double, 4>{1, 2, 3, 4});
    EXPECT_EQ(point.project(1),
              (geometry::point_t<3>(std::array<double, 3>{1, 3, 4})));
    EXPECT_THROW(point.project(4), std::out_of_range);
}

using TriangleVertices = std::array<geometry::point_t<3>, 3>;

struct TrianglePairCase {
    const char* name;
    TriangleVertices first;
    TriangleVertices second;
    bool intersects;
};

std::ostream& operator<<(std::ostream& output, const TrianglePairCase& testCase) {
    return output << testCase.name;
}

class TrianglePairIntersection : public testing::TestWithParam<TrianglePairCase> {};

TEST_P(TrianglePairIntersection,
       IsIndependentOfVertexOrderPairOrderAndProjectionAxis) {
    const auto& testCase = GetParam();
    // Cyclic coordinate permutations exercise projections onto XY, YZ and XZ.
    for (std::size_t axisRotation = 0; axisRotation < 3; ++axisRotation) {
        auto transform = [axisRotation](const geometry::point_t<3>& point) {
            const std::array<double, 3> coordinates{point.x(), point.y(), point.z()};
            return geometry::point_t<3>(
                std::array<double, 3>{coordinates.at(axisRotation),
                                      coordinates.at((axisRotation + 1) % 3),
                                      coordinates.at((axisRotation + 2) % 3)});
        };
        constexpr std::array<std::array<std::size_t, 3>, 6> VERTEX_ORDERS{
            {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}}};
        for (const auto& firstOrder : VERTEX_ORDERS) {
            for (const auto& secondOrder : VERTEX_ORDERS) {
                for (bool swapPair : {false, true}) {
                    SCOPED_TRACE(testing::Message()
                                 << "rotation=" << axisRotation
                                 << " swap=" << swapPair
                                 << " first=" << firstOrder[0] << firstOrder[1]
                                 << firstOrder[2] << " second=" << secondOrder[0]
                                 << secondOrder[1] << secondOrder[2]);
                    InitPolysData input;
                    input.data.resize(2);
                    for (std::size_t vertex = 0; vertex < 3; ++vertex) {
                        input.data[swapPair ? 1 : 0].push_back(
                            transform(testCase.first.at(firstOrder.at(vertex))));
                        input.data[swapPair ? 0 : 1].push_back(
                            transform(testCase.second.at(secondOrder.at(vertex))));
                    }
                    const auto makeTriangle = [](const auto& vertices) {
                        const auto point = [](const auto& vertex) {
                            return geometry::point_t<3>(std::array<double, 3>{
                                vertex.x(), vertex.y(), vertex.z()});
                        };
                        return geometry::triangle_t<3>(point(vertices[0]),
                                                       point(vertices[1]),
                                                       point(vertices[2]));
                    };
                    const auto first = makeTriangle(input.data[0]);
                    const auto second = makeTriangle(input.data[1]);
                    EXPECT_EQ(triangles::doTrianglesIntersect(first, second),
                              testCase.intersects);
                }
            }
        }
    }
}

constexpr double CONTAINED_VERTEX_OFFSET = 0.5;
constexpr double SEPARATED_VERTEX_X = 4.0001;

const TriangleVertices baseTriangle{{geometry::point_t<3>({0, 0, 0}),
                                     geometry::point_t<3>({4, 0, 0}),
                                     geometry::point_t<3>({0, 4, 0})}};

INSTANTIATE_TEST_SUITE_P(
    GeometricCases, TrianglePairIntersection,
    testing::Values(TrianglePairCase{"Identical", baseTriangle, baseTriangle, true},
                    TrianglePairCase{
                        "CoplanarContainment",
                        baseTriangle,
                        {{geometry::point_t<3>(
                              {CONTAINED_VERTEX_OFFSET, CONTAINED_VERTEX_OFFSET, 0}),
                          geometry::point_t<3>({1, CONTAINED_VERTEX_OFFSET, 0}),
                          geometry::point_t<3>({CONTAINED_VERTEX_OFFSET, 1, 0})}},
                        true},
                    TrianglePairCase{"CoplanarCrossingEdges",
                                     baseTriangle,
                                     {{geometry::point_t<3>({3, 3, 0}),
                                       geometry::point_t<3>({-1, 3, 0}),
                                       geometry::point_t<3>({3, -1, 0})}},
                                     true},
                    TrianglePairCase{"CoplanarVertexContact",
                                     baseTriangle,
                                     {{geometry::point_t<3>({4, 0, 0}),
                                       geometry::point_t<3>({5, 0, 0}),
                                       geometry::point_t<3>({4, 1, 0})}},
                                     true},
                    TrianglePairCase{"SharedEdge",
                                     baseTriangle,
                                     {{geometry::point_t<3>({0, 0, 0}),
                                       geometry::point_t<3>({4, 0, 0}),
                                       geometry::point_t<3>({0, -4, 0})}},
                                     true},
                    TrianglePairCase{"PartialEdgeContact",
                                     baseTriangle,
                                     {{geometry::point_t<3>({1, 0, 0}),
                                       geometry::point_t<3>({3, 0, 0}),
                                       geometry::point_t<3>({2, -2, 0})}},
                                     true},
                    TrianglePairCase{"CoplanarSeparatedWithOverlappingBounds",
                                     baseTriangle,
                                     {{geometry::point_t<3>({3, 3, 0}),
                                       geometry::point_t<3>({5, 3, 0}),
                                       geometry::point_t<3>({3, 5, 0})}},
                                     false},
                    TrianglePairCase{
                        "CoplanarSmallGap",
                        baseTriangle,
                        {{geometry::point_t<3>({SEPARATED_VERTEX_X, 0, 0}),
                          geometry::point_t<3>({5, 0, 0}),
                          geometry::point_t<3>({SEPARATED_VERTEX_X, 1, 0})}},
                        false},
                    TrianglePairCase{"ParallelPlanes",
                                     baseTriangle,
                                     {{geometry::point_t<3>({0, 0, 1}),
                                       geometry::point_t<3>({4, 0, 1}),
                                       geometry::point_t<3>({0, 4, 1})}},
                                     false},
                    TrianglePairCase{"SpatialCrossing",
                                     baseTriangle,
                                     {{geometry::point_t<3>({1, 1, -1}),
                                       geometry::point_t<3>({1, 1, 1}),
                                       geometry::point_t<3>({2, 1, 1})}},
                                     true},
                    TrianglePairCase{"SpatialVertexContact",
                                     baseTriangle,
                                     {{geometry::point_t<3>({1, 1, 0}),
                                       geometry::point_t<3>({1, 1, 2}),
                                       geometry::point_t<3>({2, 1, 2})}},
                                     true},
                    TrianglePairCase{"SpatialEdgeContact",
                                     baseTriangle,
                                     {{geometry::point_t<3>({1, 1, 0}),
                                       geometry::point_t<3>({2, 1, 0}),
                                       geometry::point_t<3>({1, 1, 2})}},
                                     true},
                    TrianglePairCase{"SpatialSharedVertex",
                                     baseTriangle,
                                     {{geometry::point_t<3>({0, 0, 0}),
                                       geometry::point_t<3>({-1, 0, 1}),
                                       geometry::point_t<3>({0, -1, 1})}},
                                     true},
                    TrianglePairCase{"CrossesPlaneOutsideTriangle",
                                     baseTriangle,
                                     {{geometry::point_t<3>({5, 5, -1}),
                                       geometry::point_t<3>({5, 5, 1}),
                                       geometry::point_t<3>({6, 5, 1})}},
                                     false},
                    TrianglePairCase{"DisjointIntervalsOnPlaneIntersection",
                                     baseTriangle,
                                     {{geometry::point_t<3>({1, 5, -1}),
                                       geometry::point_t<3>({1, 5, 1}),
                                       geometry::point_t<3>({1, 6, 0})}},
                                     false},
                    TrianglePairCase{"ObliqueCoplanarContainment",
                                     {{geometry::point_t<3>({0, 0, 0}),
                                       geometry::point_t<3>({4, 0, 4}),
                                       geometry::point_t<3>({0, 4, 4})}},
                                     {{geometry::point_t<3>({1, 1, 2}),
                                       geometry::point_t<3>({2, 1, 3}),
                                       geometry::point_t<3>({1, 2, 3})}},
                                     true}),
    [](const testing::TestParamInfo<TrianglePairCase>& info) {
        return std::string(info.param.name);
    });

} // namespace
