#include <array>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "polygonProcessor/polyWorldCalculator/polyWorldCalculator.hpp"

namespace {

using triangles::InitPolysData;
using triangles::PolyWorld;
using triangles::PolyWorldCalculator;

TEST(PolyWorld, PreservesPolygonVertices) {
    const PolyWorld<3> world(InitPolysData{{{{0, 0, 0}, {4, 0, 0}, {0, 4, 0}}}});
    ASSERT_EQ(world.polys().size(), 1U);
    const auto& polygon = world.polys()[0];
    ASSERT_EQ(polygon.numOfPoints(), 3U);
    EXPECT_DOUBLE_EQ(polygon.point(1).x(), 4);
    EXPECT_DOUBLE_EQ(polygon.point(2).y(), 4);
}

TEST(PolyWorldCalculator, HandlesEmptyAndSingleTriangle) {
    PolyWorldCalculator calculator;
    const PolyWorld<3> empty(InitPolysData{});
    const auto emptyResult = calculator.calculateIntersections(empty);
    EXPECT_EQ(&emptyResult.world, &empty);
    EXPECT_TRUE(emptyResult.intersectingPolys.empty());
    EXPECT_TRUE(emptyResult.nonIntersectingPolys.empty());
    const PolyWorld<3> single(InitPolysData{{{{0, 0, 0}, {4, 0, 0}, {0, 4, 0}}}});
    const auto singleResult = calculator.calculateIntersections(single);
    EXPECT_EQ(&singleResult.world, &single);
    EXPECT_TRUE(singleResult.intersectingPolys.empty());
    EXPECT_EQ(singleResult.nonIntersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{&single.polys()[0]}));
}

TEST(PolyWorldCalculator, PartitionsPointersIntoOriginalWorld) {
    const PolyWorld<3> world(
        InitPolysData{{{{0, 0, 0}, {4, 0, 0}, {0, 4, 0}},
                       {{1, 1, -1}, {1, 1, 1}, {2, 1, 1}},
                       {{10, 10, 0}, {11, 10, 0}, {10, 11, 0}}}});
    const auto* original = world.polys().data();
    const auto result = PolyWorldCalculator{}.calculateIntersections(world);
    EXPECT_EQ(&result.world, &world);
    EXPECT_EQ(result.intersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{&world.polys()[0],
                                                          &world.polys()[1]}));
    EXPECT_EQ(result.nonIntersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{&world.polys()[2]}));
    EXPECT_EQ(world.polys().data(), original);
    const auto repeated = PolyWorldCalculator{}.calculateIntersections(world);
    EXPECT_EQ(repeated.intersectingPolys, result.intersectingPolys);
    EXPECT_EQ(repeated.nonIntersectingPolys, result.nonIntersectingPolys);
}

TEST(PolyWorldCalculator, ReportsEachIntersectingPolygonOnce) {
    InitPolysData input;
    for (int i = 0; i < 4; ++i) {
        input.data.push_back({{0, 0, 0}, {4, 0, 0}, {0, 4, 0}});
    }
    const PolyWorld<3> world(input);
    const auto result = PolyWorldCalculator{}.calculateIntersections(world);
    EXPECT_TRUE(result.nonIntersectingPolys.empty());
    ASSERT_EQ(result.intersectingPolys.size(), world.polys().size());
    for (std::size_t i = 0; i < world.polys().size(); ++i) {
        EXPECT_EQ(result.intersectingPolys[i], &world.polys()[i]);
    }
}

TEST(PolyWorldCalculator, RejectsUnsupportedPolygonIncludingLastElement) {
    InitPolysData input{{{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}}}};
    const PolyWorld<3> quad(input);
    EXPECT_THROW(PolyWorldCalculator{}.calculateIntersections(quad),
                 std::invalid_argument);

    input.data.insert(input.data.begin(), {{0, 0, 0}, {4, 0, 0}, {0, 4, 0}});
    const PolyWorld<3> mixed(input);
    EXPECT_THROW(PolyWorldCalculator{}.calculateIntersections(mixed),
                 std::invalid_argument);
}

TEST(PolyWorldCalculator, HandlesDisjointCoplanarTriangles) {
    const PolyWorld<3> world(InitPolysData{
        {{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}, {{2, 2, 0}, {3, 2, 0}, {2, 3, 0}}}});
    const auto result = PolyWorldCalculator{}.calculateIntersections(world);
    EXPECT_TRUE(result.intersectingPolys.empty());
    EXPECT_EQ(result.nonIntersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{&world.polys()[0],
                                                          &world.polys()[1]}));
}

TEST(PolyWorldCalculator, HandlesTrianglesOnParallelPlanes) {
    const PolyWorld<3> world(InitPolysData{
        {{{0, 0, 0}, {4, 0, 0}, {0, 4, 0}}, {{0, 0, 1}, {4, 0, 1}, {0, 4, 1}}}});
    const auto result = PolyWorldCalculator{}.calculateIntersections(world);
    EXPECT_TRUE(result.intersectingPolys.empty());
    EXPECT_EQ(result.nonIntersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{&world.polys()[0],
                                                          &world.polys()[1]}));
}

TEST(PolyWorldCalculator, PartitionsInterleavedIndependentGroupsInInputOrder) {
    const PolyWorld<3> world(InitPolysData{{{{-10, 0, 0}, {-9, 0, 0}, {-10, 1, 0}},
                                            {{0, 0, 0}, {4, 0, 0}, {0, 4, 0}},
                                            {{20, 0, 0}, {24, 0, 0}, {20, 4, 0}},
                                            {{10, 0, 0}, {11, 0, 0}, {10, 1, 0}},
                                            {{1, 1, -1}, {1, 1, 1}, {2, 1, 1}},
                                            {{21, 1, -1}, {21, 1, 1}, {22, 1, 1}},
                                            {{30, 0, 0}, {31, 0, 0}, {30, 1, 0}}}});
    const auto result = PolyWorldCalculator{}.calculateIntersections(world);
    EXPECT_EQ(&result.world, &world);
    EXPECT_EQ(result.intersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{&world.polys()[1],
                                                          &world.polys()[2],
                                                          &world.polys()[4],
                                                          &world.polys()[5]}));
    EXPECT_EQ(result.nonIntersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{
                  &world.polys()[0], &world.polys()[3], &world.polys()[6]}));
}

} // namespace
