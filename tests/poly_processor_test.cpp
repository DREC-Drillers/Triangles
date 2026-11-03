#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "polygonProcessor/polyProcessor.hpp"

namespace {

using World = triangles::PolyWorld<3>;
using Processor = triangles::PolyProcessor<3>;
using Polygons = std::vector<geometry::polygon_t>;

static_assert(std::is_copy_constructible_v<World>);
static_assert(std::is_move_constructible_v<World>);
static_assert(
    std::is_const_v<
        std::remove_reference_t<decltype(std::declval<const World&>().polys()[0])>>);
static_assert(std::is_constructible_v<Processor, const World&>);
static_assert(!std::is_constructible_v<Processor, World&&>);
static_assert(!std::is_constructible_v<Processor, const World&&>);
static_assert(std::is_same_v<
              decltype(std::declval<const Processor&>().calculateIntersections()),
              triangles::IntersectionData>);

World makeWorld() {
    return World(
        triangles::InitPolysData{{{{0, 0, 0}, {2, 0, 0}, {0, 2, 0}},
                                  {{0, 0, 1}, {2, 0, 1}, {2, 2, 1}, {0, 2, 1}}}});
}

// The production calculation algorithm is implemented separately by PolyCalculator.
class RecordingCalculator {
public:
    int calls = 0;
    const Polygons* seen = nullptr;
    triangles::IntersectionData result;

    triangles::IntersectionData calculateIntersections(const Polygons& polygons) {
        ++calls;
        seen = &polygons;
        return result;
    }
};

TEST(PolyProcessor, BorrowsWorldAndForwardsExactCollectionToCalculator) {
    const World world = makeWorld();
    const Processor processor(world);
    const auto* first = &world.polys()[0];
    RecordingCalculator calculator;
    const auto result = processor.calculateIntersections(calculator);
    EXPECT_EQ(&processor.world(), &world);
    EXPECT_EQ(calculator.calls, 1);
    EXPECT_EQ(calculator.seen, &world.polys());
    EXPECT_EQ(&world.polys()[0], first);
    EXPECT_DOUBLE_EQ(world.polys()[0].area(), 2);
    EXPECT_DOUBLE_EQ(world.polys()[1].area(), 4);
    EXPECT_TRUE(result.intersections.empty());
}

TEST(PolyProcessor, ReturnsIntersectionDataWithoutRequiringPolygonGeometry) {
    const World world = makeWorld();
    const Processor processor(world);
    RecordingCalculator calculator;
    calculator.result.intersections.push_back(
        {0, 1, {triangles::PolyIntersection::Kind::POINT, {{1, 0, 0}}}});
    calculator.result.intersections.push_back(
        {0,
         1,
         {triangles::PolyIntersection::Kind::SEGMENT, {{0, 0, 0}, {1, 0, 0}}}});
    const auto result = processor.calculateIntersections(calculator);
    ASSERT_EQ(result.intersections.size(), 2U);
    EXPECT_EQ(result.intersections[0].firstPolyIndex, 0U);
    EXPECT_EQ(result.intersections[0].secondPolyIndex, 1U);
    EXPECT_EQ(result.intersections[0].intersection.kind,
              triangles::PolyIntersection::Kind::POINT);
    ASSERT_EQ(result.intersections[0].intersection.points.size(), 1U);
    EXPECT_DOUBLE_EQ(result.intersections[0].intersection.points[0].x(), 1);
    EXPECT_EQ(result.intersections[1].intersection.kind,
              triangles::PolyIntersection::Kind::SEGMENT);
    EXPECT_EQ(result.intersections[1].intersection.points.size(), 2U);
}

TEST(PolyProcessor, RecalculatesAndReturnsIndependentResults) {
    const World world = makeWorld();
    const Processor processor(world);
    RecordingCalculator calculator;
    calculator.result.intersections.push_back(
        {0, 1, {triangles::PolyIntersection::Kind::POINT, {{1, 0, 0}}}});
    const auto first = processor.calculateIntersections(calculator);
    calculator.result.intersections.clear();
    const auto second = processor.calculateIntersections(calculator);
    EXPECT_EQ(calculator.calls, 2);
    EXPECT_EQ(first.intersections.size(), 1U);
    EXPECT_TRUE(second.intersections.empty());
}

TEST(PolyProcessor, ForwardsEmptyWorld) {
    const World world(triangles::InitPolysData{});
    const Processor processor(world);
    RecordingCalculator calculator;
    const auto result = processor.calculateIntersections(calculator);
    EXPECT_EQ(calculator.calls, 1);
    EXPECT_EQ(calculator.seen, &world.polys());
    EXPECT_TRUE(result.intersections.empty());
}

class FailingCalculator {
public:
    triangles::IntersectionData calculateIntersections(const Polygons&) {
        throw std::runtime_error("calculation failed");
    }
};

TEST(PolyProcessor, PropagatesCalculatorErrorsAndKeepsWorldOwned) {
    const World world = makeWorld();
    const Processor processor(world);
    const auto* first = &world.polys()[0];
    FailingCalculator calculator;
    EXPECT_THROW(processor.calculateIntersections(calculator), std::runtime_error);
    ASSERT_EQ(world.polys().size(), 2U);
    EXPECT_EQ(&world.polys()[0], first);
}

} // namespace
