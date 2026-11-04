#include <array>
#include <ostream>
#include <string>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "polygonProcessor/polyCalculator.hpp"

namespace {

using triangles::InitPolysData;
using triangles::PolyCalculator;
using triangles::PolyWorld;

template <typename Point>
concept HasProjection = requires(const Point& point) { point.project(0); };

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
    EXPECT_EQ(point.project(1), (geometry::point_t<3>(std::array<double, 3>{1, 3, 4})));
    EXPECT_THROW(point.project(4), std::out_of_range);
}

TEST(PolyWorld, PreservesPolygonVertices) {
    const PolyWorld<3> world(InitPolysData{
        {{{0, 0, 0}, {4, 0, 0}, {0, 4, 0}}}});
    ASSERT_EQ(world.polys().size(), 1U);
    const auto& polygon = world.polys()[0];
    ASSERT_EQ(polygon.numOfPoints(), 3U);
    EXPECT_DOUBLE_EQ(polygon.point(1).x(), 4);
    EXPECT_DOUBLE_EQ(polygon.point(2).y(), 4);
}

TEST(PolyCalculator, HandlesEmptyAndSingleTriangle) {
    PolyCalculator calculator;
    const PolyWorld<3> empty(InitPolysData{});
    const auto emptyResult = calculator.calculateIntersections(empty);
    EXPECT_EQ(&emptyResult.world, &empty);
    EXPECT_TRUE(emptyResult.intersectingPolys.empty());
    EXPECT_TRUE(emptyResult.nonIntersectingPolys.empty());
    const PolyWorld<3> single(InitPolysData{
        {{{0, 0, 0}, {4, 0, 0}, {0, 4, 0}}}});
    const auto singleResult = calculator.calculateIntersections(single);
    EXPECT_EQ(&singleResult.world, &single);
    EXPECT_TRUE(singleResult.intersectingPolys.empty());
    EXPECT_EQ(singleResult.nonIntersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{&single.polys()[0]}));
}

TEST(PolyCalculator, PartitionsPointersIntoOriginalWorld) {
    const PolyWorld<3> world(InitPolysData{
        {{{0, 0, 0}, {4, 0, 0}, {0, 4, 0}},
         {{1, 1, -1}, {1, 1, 1}, {2, 1, 1}},
         {{10, 10, 0}, {11, 10, 0}, {10, 11, 0}}}});
    const auto* original = world.polys().data();
    const auto result = PolyCalculator{}.calculateIntersections(world);
    EXPECT_EQ(&result.world, &world);
    EXPECT_EQ(result.intersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{&world.polys()[0],
                                                       &world.polys()[1]}));
    EXPECT_EQ(result.nonIntersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{&world.polys()[2]}));
    EXPECT_EQ(world.polys().data(), original);
    const auto repeated = PolyCalculator{}.calculateIntersections(world);
    EXPECT_EQ(repeated.intersectingPolys, result.intersectingPolys);
    EXPECT_EQ(repeated.nonIntersectingPolys, result.nonIntersectingPolys);
}

TEST(PolyCalculator, ReportsEachIntersectingPolygonOnce) {
    InitPolysData input;
    for (int i = 0; i < 4; ++i) {
        input.data.push_back({{0, 0, 0}, {4, 0, 0}, {0, 4, 0}});
    }
    const PolyWorld<3> world(input);
    const auto result = PolyCalculator{}.calculateIntersections(world);
    EXPECT_TRUE(result.nonIntersectingPolys.empty());
    ASSERT_EQ(result.intersectingPolys.size(), world.polys().size());
    for (std::size_t i = 0; i < world.polys().size(); ++i) {
        EXPECT_EQ(result.intersectingPolys[i], &world.polys()[i]);
    }
}

TEST(PolyCalculator, RejectsUnsupportedPolygonIncludingLastElement) {
    InitPolysData input{
        {{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}}}};
    const PolyWorld<3> quad(input);
    EXPECT_THROW(PolyCalculator{}.calculateIntersections(quad),
                 std::invalid_argument);

    input.data.insert(input.data.begin(),
                      {{0, 0, 0}, {4, 0, 0}, {0, 4, 0}});
    const PolyWorld<3> mixed(input);
    EXPECT_THROW(PolyCalculator{}.calculateIntersections(mixed),
                 std::invalid_argument);
}

TEST(PolyCalculator, HandlesDisjointCoplanarTriangles) {
    const PolyWorld<3> world(InitPolysData{
        {{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}},
         {{2, 2, 0}, {3, 2, 0}, {2, 3, 0}}}});
    const auto result = PolyCalculator{}.calculateIntersections(world);
    EXPECT_TRUE(result.intersectingPolys.empty());
    EXPECT_EQ(result.nonIntersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{&world.polys()[0],
                                                       &world.polys()[1]}));
}

TEST(PolyCalculator, HandlesTrianglesOnParallelPlanes) {
    const PolyWorld<3> world(InitPolysData{
        {{{0, 0, 0}, {4, 0, 0}, {0, 4, 0}},
         {{0, 0, 1}, {4, 0, 1}, {0, 4, 1}}}});
    const auto result = PolyCalculator{}.calculateIntersections(world);
    EXPECT_TRUE(result.intersectingPolys.empty());
    EXPECT_EQ(result.nonIntersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{&world.polys()[0],
                                                       &world.polys()[1]}));
}


using TriangleVertices = std::array<triangles::Point, 3>;

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

TEST_P(TrianglePairIntersection, IsIndependentOfVertexOrderPairOrderAndProjectionAxis) {
    const auto& testCase = GetParam();
    // Cyclic coordinate permutations exercise projections onto XY, YZ and XZ.
    for (std::size_t axisRotation = 0; axisRotation < 3; ++axisRotation) {
        auto transform = [axisRotation](const triangles::Point& point) {
            const std::array<double, 3> coordinates{point.x(), point.y(), point.z()};
            return triangles::Point(coordinates.at(axisRotation),
                                    coordinates.at((axisRotation + 1) % 3),
                                    coordinates.at((axisRotation + 2) % 3));
        };
        constexpr std::array<std::array<std::size_t, 3>, 6> VERTEX_ORDERS{{
            {0, 1, 2}, {0, 2, 1}, {1, 0, 2},
            {1, 2, 0}, {2, 0, 1}, {2, 1, 0}}};
        for (const auto& firstOrder : VERTEX_ORDERS) {
            for (const auto& secondOrder : VERTEX_ORDERS) {
                for (bool swapPair : {false, true}) {
                    SCOPED_TRACE(testing::Message()
                                 << "rotation=" << axisRotation << " swap=" << swapPair
                                 << " first=" << firstOrder[0] << firstOrder[1] << firstOrder[2]
                                 << " second=" << secondOrder[0] << secondOrder[1] << secondOrder[2]);
                    InitPolysData input;
                    input.data.resize(2);
                    for (std::size_t vertex = 0; vertex < 3; ++vertex) {
                        input.data[swapPair ? 1 : 0].push_back(
                            transform(testCase.first.at(firstOrder.at(vertex))));
                        input.data[swapPair ? 0 : 1].push_back(
                            transform(testCase.second.at(secondOrder.at(vertex))));
                    }
                    const PolyWorld<3> world(input);
                    const auto result = PolyCalculator{}.calculateIntersections(world);
                    const std::vector<const geometry::polygon_t<3>*> both{
                        &world.polys()[0], &world.polys()[1]};
                    const std::vector<const geometry::polygon_t<3>*> none;
                    EXPECT_EQ(&result.world, &world);
                    EXPECT_EQ(result.intersectingPolys, testCase.intersects ? both : none);
                    EXPECT_EQ(result.nonIntersectingPolys, testCase.intersects ? none : both);
                }
            }
        }
    }
}

constexpr double CONTAINED_VERTEX_OFFSET = 0.5;
constexpr double SEPARATED_VERTEX_X = 4.0001;

const TriangleVertices baseTriangle{{{0, 0, 0}, {4, 0, 0}, {0, 4, 0}}};

INSTANTIATE_TEST_SUITE_P(
    GeometricCases, TrianglePairIntersection,
    testing::Values(
        TrianglePairCase{"Identical", baseTriangle, baseTriangle, true},
        TrianglePairCase{"CoplanarContainment", baseTriangle,
                         {{{CONTAINED_VERTEX_OFFSET, CONTAINED_VERTEX_OFFSET, 0},
                           {1, CONTAINED_VERTEX_OFFSET, 0},
                           {CONTAINED_VERTEX_OFFSET, 1, 0}}}, true},
        TrianglePairCase{"CoplanarCrossingEdges", baseTriangle,
                         {{{3, 3, 0}, {-1, 3, 0}, {3, -1, 0}}}, true},
        TrianglePairCase{"CoplanarVertexContact", baseTriangle,
                         {{{4, 0, 0}, {5, 0, 0}, {4, 1, 0}}}, true},
        TrianglePairCase{"SharedEdge", baseTriangle,
                         {{{0, 0, 0}, {4, 0, 0}, {0, -4, 0}}}, true},
        TrianglePairCase{"PartialEdgeContact", baseTriangle,
                         {{{1, 0, 0}, {3, 0, 0}, {2, -2, 0}}}, true},
        TrianglePairCase{"CoplanarSeparatedWithOverlappingBounds", baseTriangle,
                         {{{3, 3, 0}, {5, 3, 0}, {3, 5, 0}}}, false},
        TrianglePairCase{"CoplanarSmallGap", baseTriangle,
                         {{{SEPARATED_VERTEX_X, 0, 0}, {5, 0, 0}, {SEPARATED_VERTEX_X, 1, 0}}}, false},
        TrianglePairCase{"ParallelPlanes", baseTriangle,
                         {{{0, 0, 1}, {4, 0, 1}, {0, 4, 1}}}, false},
        TrianglePairCase{"SpatialCrossing", baseTriangle,
                         {{{1, 1, -1}, {1, 1, 1}, {2, 1, 1}}}, true},
        TrianglePairCase{"SpatialVertexContact", baseTriangle,
                         {{{1, 1, 0}, {1, 1, 2}, {2, 1, 2}}}, true},
        TrianglePairCase{"SpatialEdgeContact", baseTriangle,
                         {{{1, 1, 0}, {2, 1, 0}, {1, 1, 2}}}, true},
        TrianglePairCase{"SpatialSharedVertex", baseTriangle,
                         {{{0, 0, 0}, {-1, 0, 1}, {0, -1, 1}}}, true},
        TrianglePairCase{"CrossesPlaneOutsideTriangle", baseTriangle,
                         {{{5, 5, -1}, {5, 5, 1}, {6, 5, 1}}}, false},
        TrianglePairCase{"DisjointIntervalsOnPlaneIntersection", baseTriangle,
                         {{{1, 5, -1}, {1, 5, 1}, {1, 6, 0}}}, false},
        TrianglePairCase{"ObliqueCoplanarContainment",
                         {{{0, 0, 0}, {4, 0, 4}, {0, 4, 4}}},
                         {{{1, 1, 2}, {2, 1, 3}, {1, 2, 3}}}, true}),
    [](const testing::TestParamInfo<TrianglePairCase>& info) {
        return std::string(info.param.name);
    });

TEST(PolyCalculator, PartitionsInterleavedIndependentGroupsInInputOrder) {
    const PolyWorld<3> world(InitPolysData{
        {{{-10, 0, 0}, {-9, 0, 0}, {-10, 1, 0}},
         {{0, 0, 0}, {4, 0, 0}, {0, 4, 0}},
         {{20, 0, 0}, {24, 0, 0}, {20, 4, 0}},
         {{10, 0, 0}, {11, 0, 0}, {10, 1, 0}},
         {{1, 1, -1}, {1, 1, 1}, {2, 1, 1}},
         {{21, 1, -1}, {21, 1, 1}, {22, 1, 1}},
         {{30, 0, 0}, {31, 0, 0}, {30, 1, 0}}}});
    const auto result = PolyCalculator{}.calculateIntersections(world);
    EXPECT_EQ(&result.world, &world);
    EXPECT_EQ(result.intersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{
                  &world.polys()[1], &world.polys()[2],
                  &world.polys()[4], &world.polys()[5]}));
    EXPECT_EQ(result.nonIntersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{
                  &world.polys()[0], &world.polys()[3], &world.polys()[6]}));
}

} // namespace
