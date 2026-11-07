#include <stdexcept>

#include <gtest/gtest.h>

#include "polygonProcessor/polyWorld.hpp"
#include "polygonProcessor/polyWorldCalculator/polyCalculator.hpp"

namespace {

using triangles::InitPolysData;
using triangles::PolyCalculator;
using triangles::PolyWorld;

TEST(PolyCalculator, ChecksIntersectingPolygons) {
    const PolyWorld<3> world(InitPolysData{
        {{{0, 0, 0}, {4, 0, 0}, {0, 4, 0}}, {{1, 1, -1}, {1, 1, 1}, {2, 1, 1}}}});

    EXPECT_TRUE(PolyCalculator{}.intersects(world.polys()[0], world.polys()[1]));
}

TEST(PolyCalculator, ChecksDisjointPolygons) {
    const PolyWorld<3> world(InitPolysData{
        {{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}, {{2, 2, 0}, {3, 2, 0}, {2, 3, 0}}}});

    EXPECT_FALSE(PolyCalculator{}.intersects(world.polys()[0], world.polys()[1]));
}

TEST(PolyCalculator, RejectsUnsupportedPolygonInEitherPosition) {
    const PolyWorld<3> world(
        InitPolysData{{{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}},
                       {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}}}});

    EXPECT_THROW(PolyCalculator{}.intersects(world.polys()[0], world.polys()[1]),
                 std::invalid_argument);
    EXPECT_THROW(PolyCalculator{}.intersects(world.polys()[1], world.polys()[0]),
                 std::invalid_argument);
}

} // namespace
