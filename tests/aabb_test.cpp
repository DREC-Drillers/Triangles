#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>

#include <gtest/gtest.h>

#include "polygonProcessor/polyWorldCalculator/spatial/aabb.hpp"

namespace {

using triangles::Aabb;

TEST(Aabb, PreservesBoundsAndComputesCenter) {
    const Aabb box({-2, 1, 0}, {4, 5, 8});

    EXPECT_EQ(box.min(), (Aabb::Coordinates{-2, 1, 0}));
    EXPECT_EQ(box.max(), (Aabb::Coordinates{4, 5, 8}));
    EXPECT_EQ(box.center(), (Aabb::Coordinates{1, 3, 4}));
}

TEST(Aabb, RejectsReversedAndNonFiniteBoundsOnEveryAxis) {
    constexpr double INFINITY_VALUE = std::numeric_limits<double>::infinity();
    constexpr double NAN_VALUE = std::numeric_limits<double>::quiet_NaN();

    for (std::size_t axis = 0; axis < 3; ++axis) {
        Aabb::Coordinates lower{0, 0, 0};
        Aabb::Coordinates upper{1, 1, 1};
        lower.at(axis) = 2;
        EXPECT_THROW((Aabb(lower, upper)), std::invalid_argument);

        for (double invalid : {INFINITY_VALUE, -INFINITY_VALUE, NAN_VALUE}) {
            lower = {0, 0, 0};
            upper = {1, 1, 1};
            lower.at(axis) = invalid;
            EXPECT_THROW((Aabb(lower, upper)), std::invalid_argument);

            lower = {0, 0, 0};
            upper.at(axis) = invalid;
            EXPECT_THROW((Aabb(lower, upper)), std::invalid_argument);
        }
    }
}

TEST(Aabb, DetectsOverlapContainmentAndSelfOverlap) {
    const Aabb outer({0, 0, 0}, {4, 4, 4});
    const Aabb inner({1, 1, 1}, {2, 2, 2});
    const Aabb crossing({3, -1, 2}, {5, 2, 6});

    EXPECT_TRUE(outer.overlaps(inner));
    EXPECT_TRUE(inner.overlaps(outer));
    EXPECT_TRUE(outer.overlaps(crossing));
    EXPECT_TRUE(crossing.overlaps(outer));
    EXPECT_TRUE(outer.overlaps(outer));
}

TEST(Aabb, RejectsSeparationInEitherDirectionOnEveryAxis) {
    const Aabb box({0, 0, 0}, {1, 1, 1});

    for (std::size_t axis = 0; axis < 3; ++axis) {
        Aabb::Coordinates lower{0, 0, 0};
        Aabb::Coordinates upper{1, 1, 1};
        lower.at(axis) = 2;
        upper.at(axis) = 3;
        const Aabb separated(lower, upper);

        EXPECT_FALSE(box.overlaps(separated));
        EXPECT_FALSE(separated.overlaps(box));
    }
}

TEST(Aabb, IncludesFaceEdgeAndCornerContact) {
    const Aabb box({0, 0, 0}, {1, 1, 1});

    EXPECT_TRUE(box.overlaps(Aabb({1, 0, 0}, {2, 1, 1})));
    EXPECT_TRUE(box.overlaps(Aabb({1, 1, 0}, {2, 2, 1})));
    EXPECT_TRUE(box.overlaps(Aabb({1, 1, 1}, {2, 2, 2})));
}

TEST(Aabb, SupportsFlatBoxesAndPointsWithoutImplicitTolerance) {
    const Aabb plane({0, 0, 0}, {2, 2, 0});
    const Aabb point({1, 1, 0}, {1, 1, 0});
    const Aabb offset({0, 0, 1e-12}, {2, 2, 1e-12});

    EXPECT_TRUE(plane.overlaps(point));
    EXPECT_TRUE(point.overlaps(plane));
    EXPECT_TRUE(point.overlaps(point));
    EXPECT_FALSE(plane.overlaps(offset));
    EXPECT_EQ(point.center(), point.min());
}

TEST(Aabb, ExpandsToEncloseBothBoxesAndPreservesContainingBounds) {
    Aabb box({0, 0, 0}, {2, 2, 2});
    box.expand(Aabb({-3, 1, -1}, {1, 5, 1}));

    EXPECT_EQ(box.min(), (Aabb::Coordinates{-3, 0, -1}));
    EXPECT_EQ(box.max(), (Aabb::Coordinates{2, 5, 2}));

    box.expand(Aabb({0, 1, 0}, {1, 2, 1}));
    box.expand(box);
    EXPECT_EQ(box.min(), (Aabb::Coordinates{-3, 0, -1}));
    EXPECT_EQ(box.max(), (Aabb::Coordinates{2, 5, 2}));
}

TEST(Aabb, ComputesFiniteCenterForExtremeBounds) {
    constexpr double MAX_VALUE = std::numeric_limits<double>::max();
    const Aabb box({-MAX_VALUE, MAX_VALUE / 2, -MAX_VALUE},
                   {MAX_VALUE, MAX_VALUE, -MAX_VALUE / 2});
    const auto center = box.center();

    EXPECT_DOUBLE_EQ(center[0], 0);
    EXPECT_DOUBLE_EQ(center[1], MAX_VALUE * 0.75);
    EXPECT_DOUBLE_EQ(center[2], -MAX_VALUE * 0.75);
    for (double coordinate : center) {
        EXPECT_TRUE(std::isfinite(coordinate));
    }
}

} // namespace
