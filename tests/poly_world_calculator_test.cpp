#include <array>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "polygonProcessor/polyWorldCalculator/intersections/triangleIntersection.hpp"
#include "polygonProcessor/polyWorldCalculator/polyWorldCalculator.hpp"

namespace {

using triangles::InitPolysData;
using triangles::PolyWorld;
using triangles::PolyWorldCalculator;

TEST(PolyWorldCalculator, RejectsSeparatedBoundsDespiteOrientationTolerance) {
    const PolyWorld world(InitPolysData{{{geometry::point_t<3>({0, 0, 0}),
                                          geometry::point_t<3>({1, 0, 0}),
                                          geometry::point_t<3>({0, 1, 0})},
                                         {geometry::point_t<3>({0, 0, 1e-12}),
                                          geometry::point_t<3>({1, 0, 1e-12}),
                                          geometry::point_t<3>({0, 1, 1e-12})}}});

    const auto result = PolyWorldCalculator{}.calculateIntersections(world);
    EXPECT_TRUE(result.intersectingPolys.empty());
    EXPECT_EQ(result.nonIntersectingPolys.size(), 2U);
}

TEST(PolyWorldCalculator, MatchesBruteForceForMultipleSpatialGroups) {
    InitPolysData input;
    constexpr int GROUP_COUNT = 30;
    for (int group = 0; group < GROUP_COUNT; ++group) {
        const double x = group * 10;
        input.data.push_back({geometry::point_t<3>({x, 0, 0}),
                              geometry::point_t<3>({x + 2, 0, 0}),
                              geometry::point_t<3>({x, 2, 0})});
        const double offset = group % 2 ? 1 : 3;
        input.data.push_back({geometry::point_t<3>({x + offset, 0, 0}),
                              geometry::point_t<3>({x + offset + 2, 0, 0}),
                              geometry::point_t<3>({x + offset, 2, 0})});
    }
    const PolyWorld world(input);
    const auto& polygons = world.polys();
    std::vector<bool> marked(polygons.size(), false);
    for (std::size_t first = 0; first < polygons.size(); ++first) {
        for (std::size_t second = 0; second < first; ++second) {
            const auto makeTriangle = [](const auto& polygon) {
                return geometry::triangle_t<3>(
                    polygon->point(0), polygon->point(1), polygon->point(2));
            };
            const auto a = makeTriangle(polygons.at(first));
            const auto b = makeTriangle(polygons.at(second));
            if (a.aabbBounds().overlaps(b.aabbBounds()) &&
                triangles::doTrianglesIntersect(a, b)) {
                marked[first] = true;
                marked[second] = true;
            }
        }
    }

    const auto result = PolyWorldCalculator{}.calculateIntersections(world);
    std::vector<const geometry::polygon_t<3>*> intersecting;
    std::vector<const geometry::polygon_t<3>*> nonIntersecting;
    for (std::size_t id = 0; id < polygons.size(); ++id) {
        (marked[id] ? intersecting : nonIntersecting)
            .push_back(polygons.at(id).get());
    }
    EXPECT_EQ(result.intersectingPolys, intersecting);
    EXPECT_EQ(result.nonIntersectingPolys, nonIntersecting);
}

TEST(PolyWorld, PreservesPolygonVertices) {
    const PolyWorld world(InitPolysData{{{geometry::point_t<3>({0, 0, 0}),
                                          geometry::point_t<3>({4, 0, 0}),
                                          geometry::point_t<3>({0, 4, 0})}}});
    ASSERT_EQ(world.polys().size(), 1U);
    const auto& polygon = world.polys()[0];
    ASSERT_EQ(polygon->numOfPoints(), 3U);
    EXPECT_DOUBLE_EQ(polygon->point(1).x(), 4);
    EXPECT_DOUBLE_EQ(polygon->point(2).y(), 4);
}

TEST(PolyWorldCalculator, HandlesEmptyAndSingleTriangle) {
    PolyWorldCalculator calculator;
    const PolyWorld empty(InitPolysData{});
    const auto emptyResult = calculator.calculateIntersections(empty);
    EXPECT_EQ(&emptyResult.world, &empty);
    EXPECT_TRUE(emptyResult.intersectingPolys.empty());
    EXPECT_TRUE(emptyResult.nonIntersectingPolys.empty());
    const PolyWorld single(InitPolysData{{{geometry::point_t<3>({0, 0, 0}),
                                           geometry::point_t<3>({4, 0, 0}),
                                           geometry::point_t<3>({0, 4, 0})}}});
    const auto singleResult = calculator.calculateIntersections(single);
    EXPECT_EQ(&singleResult.world, &single);
    EXPECT_TRUE(singleResult.intersectingPolys.empty());
    EXPECT_EQ(singleResult.nonIntersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{single.polys()[0].get()}));
}

TEST(PolyWorldCalculator, PartitionsPointersIntoOriginalWorld) {
    const PolyWorld world(InitPolysData{{{geometry::point_t<3>({0, 0, 0}),
                                          geometry::point_t<3>({4, 0, 0}),
                                          geometry::point_t<3>({0, 4, 0})},
                                         {geometry::point_t<3>({1, 1, -1}),
                                          geometry::point_t<3>({1, 1, 1}),
                                          geometry::point_t<3>({2, 1, 1})},
                                         {geometry::point_t<3>({10, 10, 0}),
                                          geometry::point_t<3>({11, 10, 0}),
                                          geometry::point_t<3>({10, 11, 0})}}});
    const auto* original = world.polys().data();
    const auto result = PolyWorldCalculator{}.calculateIntersections(world);
    EXPECT_EQ(&result.world, &world);
    EXPECT_EQ(result.intersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{world.polys()[0].get(),
                                                          world.polys()[1].get()}));
    EXPECT_EQ(result.nonIntersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{world.polys()[2].get()}));
    EXPECT_EQ(world.polys().data(), original);
    const auto repeated = PolyWorldCalculator{}.calculateIntersections(world);
    EXPECT_EQ(repeated.intersectingPolys, result.intersectingPolys);
    EXPECT_EQ(repeated.nonIntersectingPolys, result.nonIntersectingPolys);
}

TEST(PolyWorldCalculator, ReportsEachIntersectingPolygonOnce) {
    InitPolysData input;
    for (int i = 0; i < 4; ++i) {
        input.data.push_back({geometry::point_t<3>({0, 0, 0}),
                              geometry::point_t<3>({4, 0, 0}),
                              geometry::point_t<3>({0, 4, 0})});
    }
    const PolyWorld world(input);
    const auto result = PolyWorldCalculator{}.calculateIntersections(world);
    EXPECT_TRUE(result.nonIntersectingPolys.empty());
    ASSERT_EQ(result.intersectingPolys.size(), world.polys().size());
    for (std::size_t i = 0; i < world.polys().size(); ++i) {
        EXPECT_EQ(result.intersectingPolys[i], world.polys()[i].get());
    }
}

TEST(PolyWorldCalculator, RejectsUnsupportedPolygonIncludingLastElement) {
    InitPolysData input{{{geometry::point_t<3>({0, 0, 0}),
                          geometry::point_t<3>({1, 0, 0}),
                          geometry::point_t<3>({1, 1, 0}),
                          geometry::point_t<3>({0, 1, 0})}}};
    const PolyWorld quad(input);
    EXPECT_THROW(PolyWorldCalculator{}.calculateIntersections(quad),
                 std::invalid_argument);

    input.data.insert(input.data.begin(),
                      {geometry::point_t<3>({0, 0, 0}),
                       geometry::point_t<3>({4, 0, 0}),
                       geometry::point_t<3>({0, 4, 0})});
    const PolyWorld mixed(input);
    EXPECT_THROW(PolyWorldCalculator{}.calculateIntersections(mixed),
                 std::invalid_argument);
}

TEST(PolyWorldCalculator, HandlesDisjointCoplanarTriangles) {
    const PolyWorld world(InitPolysData{{{geometry::point_t<3>({0, 0, 0}),
                                          geometry::point_t<3>({1, 0, 0}),
                                          geometry::point_t<3>({0, 1, 0})},
                                         {geometry::point_t<3>({2, 2, 0}),
                                          geometry::point_t<3>({3, 2, 0}),
                                          geometry::point_t<3>({2, 3, 0})}}});
    const auto result = PolyWorldCalculator{}.calculateIntersections(world);
    EXPECT_TRUE(result.intersectingPolys.empty());
    EXPECT_EQ(result.nonIntersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{world.polys()[0].get(),
                                                          world.polys()[1].get()}));
}

TEST(PolyWorldCalculator, HandlesTrianglesOnParallelPlanes) {
    const PolyWorld world(InitPolysData{{{geometry::point_t<3>({0, 0, 0}),
                                          geometry::point_t<3>({4, 0, 0}),
                                          geometry::point_t<3>({0, 4, 0})},
                                         {geometry::point_t<3>({0, 0, 1}),
                                          geometry::point_t<3>({4, 0, 1}),
                                          geometry::point_t<3>({0, 4, 1})}}});
    const auto result = PolyWorldCalculator{}.calculateIntersections(world);
    EXPECT_TRUE(result.intersectingPolys.empty());
    EXPECT_EQ(result.nonIntersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{world.polys()[0].get(),
                                                          world.polys()[1].get()}));
}

TEST(PolyWorldCalculator, PartitionsInterleavedIndependentGroupsInInputOrder) {
    const PolyWorld world(InitPolysData{{{geometry::point_t<3>({-10, 0, 0}),
                                          geometry::point_t<3>({-9, 0, 0}),
                                          geometry::point_t<3>({-10, 1, 0})},
                                         {geometry::point_t<3>({0, 0, 0}),
                                          geometry::point_t<3>({4, 0, 0}),
                                          geometry::point_t<3>({0, 4, 0})},
                                         {geometry::point_t<3>({20, 0, 0}),
                                          geometry::point_t<3>({24, 0, 0}),
                                          geometry::point_t<3>({20, 4, 0})},
                                         {geometry::point_t<3>({10, 0, 0}),
                                          geometry::point_t<3>({11, 0, 0}),
                                          geometry::point_t<3>({10, 1, 0})},
                                         {geometry::point_t<3>({1, 1, -1}),
                                          geometry::point_t<3>({1, 1, 1}),
                                          geometry::point_t<3>({2, 1, 1})},
                                         {geometry::point_t<3>({21, 1, -1}),
                                          geometry::point_t<3>({21, 1, 1}),
                                          geometry::point_t<3>({22, 1, 1})},
                                         {geometry::point_t<3>({30, 0, 0}),
                                          geometry::point_t<3>({31, 0, 0}),
                                          geometry::point_t<3>({30, 1, 0})}}});
    const auto result = PolyWorldCalculator{}.calculateIntersections(world);
    EXPECT_EQ(&result.world, &world);
    EXPECT_EQ(result.intersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{world.polys()[1].get(),
                                                          world.polys()[2].get(),
                                                          world.polys()[4].get(),
                                                          world.polys()[5].get()}));
    EXPECT_EQ(result.nonIntersectingPolys,
              (std::vector<const geometry::polygon_t<3>*>{world.polys()[0].get(),
                                                          world.polys()[3].get(),
                                                          world.polys()[6].get()}));
}

} // namespace

namespace {

using triangles::InitPolysData;
using triangles::PolyWorld;
using triangles::PolyWorldCalculator;

TEST(PolyWorldCalculator, ChecksIntersectingPolygons) {
    const PolyWorld world(InitPolysData{{{geometry::point_t<3>({0, 0, 0}),
                                          geometry::point_t<3>({4, 0, 0}),
                                          geometry::point_t<3>({0, 4, 0})},
                                         {geometry::point_t<3>({1, 1, -1}),
                                          geometry::point_t<3>({1, 1, 1}),
                                          geometry::point_t<3>({2, 1, 1})}}});

    EXPECT_EQ(
        PolyWorldCalculator{}.calculateIntersections(world).intersectingPolys.size(),
        2U);
}

TEST(PolyWorldCalculator, ChecksDisjointPolygons) {
    const PolyWorld world(InitPolysData{{{geometry::point_t<3>({0, 0, 0}),
                                          geometry::point_t<3>({1, 0, 0}),
                                          geometry::point_t<3>({0, 1, 0})},
                                         {geometry::point_t<3>({2, 2, 0}),
                                          geometry::point_t<3>({3, 2, 0}),
                                          geometry::point_t<3>({2, 3, 0})}}});

    EXPECT_TRUE(PolyWorldCalculator{}
                    .calculateIntersections(world)
                    .intersectingPolys.empty());
}

TEST(PolyWorldCalculator, RejectsUnsupportedPolygonInEitherPosition) {
    const PolyWorld world(InitPolysData{{{geometry::point_t<3>({0, 0, 0}),
                                          geometry::point_t<3>({1, 0, 0}),
                                          geometry::point_t<3>({0, 1, 0})},
                                         {geometry::point_t<3>({0, 0, 0}),
                                          geometry::point_t<3>({1, 0, 0}),
                                          geometry::point_t<3>({1, 1, 0}),
                                          geometry::point_t<3>({0, 1, 0})}}});

    EXPECT_THROW(PolyWorldCalculator{}.calculateIntersections(world),
                 std::invalid_argument);
    const PolyWorld reversed(InitPolysData{{{geometry::point_t<3>({0, 0, 0}),
                                             geometry::point_t<3>({1, 0, 0}),
                                             geometry::point_t<3>({1, 1, 0}),
                                             geometry::point_t<3>({0, 1, 0})},
                                            {geometry::point_t<3>({0, 0, 0}),
                                             geometry::point_t<3>({1, 0, 0}),
                                             geometry::point_t<3>({0, 1, 0})}}});
    EXPECT_THROW(PolyWorldCalculator{}.calculateIntersections(reversed),
                 std::invalid_argument);
}

} // namespace

namespace {

static_assert(std::has_virtual_destructor_v<geometry::polygon_t<3>>);
static_assert(!std::is_copy_constructible_v<PolyWorld>);
static_assert(std::is_nothrow_move_constructible_v<PolyWorld>);

TEST(PolyWorld, PreservesTriangleTypeAndSupportsOtherPolygons) {
    const PolyWorld world(InitPolysData{{{geometry::point_t<3>({0, 0, 0}),
                                          geometry::point_t<3>({1, 0, 0}),
                                          geometry::point_t<3>({0, 1, 0})},
                                         {geometry::point_t<3>({0, 0, 0}),
                                          geometry::point_t<3>({1, 0, 0}),
                                          geometry::point_t<3>({1, 1, 0}),
                                          geometry::point_t<3>({0, 1, 0})}}});
    ASSERT_NE(dynamic_cast<const geometry::triangle_t<3>*>(world.polys()[0].get()),
              nullptr);
    EXPECT_EQ(dynamic_cast<const geometry::triangle_t<3>*>(world.polys()[1].get()),
              nullptr);
    EXPECT_EQ(world.polys()[1]->numOfPoints(), 4U);
}

TEST(PolyWorld, MovingPreservesOwnedPolygonAddresses) {
    PolyWorld source(InitPolysData{{{geometry::point_t<3>({0, 0, 0}),
                                     geometry::point_t<3>({1, 0, 0}),
                                     geometry::point_t<3>({0, 1, 0})}}});
    const auto* polygon = source.polys()[0].get();
    const PolyWorld destination(std::move(source));
    EXPECT_EQ(destination.polys()[0].get(), polygon);
    const auto result = PolyWorldCalculator{}.calculateIntersections(destination);
    ASSERT_EQ(result.nonIntersectingPolys.size(), 1U);
    EXPECT_EQ(result.nonIntersectingPolys[0], polygon);
}

TEST(PolyWorld, BasePointerDestroysDerivedPolygon) {
    class PolygonProbe final : public geometry::polygon_t<3> {
    public:
        explicit PolygonProbe(bool& destroyed)
            : geometry::polygon_t<3>({geometry::point_t<3>({0, 0, 0}),
                                      geometry::point_t<3>({1, 0, 0}),
                                      geometry::point_t<3>({0, 1, 0})}),
              destroyed_(destroyed) {}
        ~PolygonProbe() override { destroyed_ = true; }
        PolygonProbe(const PolygonProbe&) = delete;
        PolygonProbe& operator=(const PolygonProbe&) = delete;
        PolygonProbe(PolygonProbe&&) = delete;
        PolygonProbe& operator=(PolygonProbe&&) = delete;

    private:
        bool& destroyed_;
    };
    bool destroyed = false;
    {
        std::unique_ptr<geometry::polygon_t<3>> polygon =
            std::make_unique<PolygonProbe>(destroyed);
    }
    EXPECT_TRUE(destroyed);
}

} // namespace
