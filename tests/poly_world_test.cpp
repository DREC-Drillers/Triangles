#include <stdexcept>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include "lexer/lexer.h"
#include "parser/parser.hpp"
#include "polygonProcessor/polyProcessor.hpp"

namespace {

using TriangleWorld = triangles::PolyWorld<3>;
using TriangleProcessor = triangles::PolyProcessor<3>;

static_assert(
    std::is_same_v<decltype(std::declval<const TriangleWorld&>().polys()[0]),
                   const geometry::polygon_t&>);

triangles::InitPolysData triangleData() {
    return {{{{0, 0, 0}, {2, 0, 0}, {0, 2, 0}}, {{0, 0, 5}, {3, 0, 5}, {0, 3, 5}}}};
}

TEST(PolyWorld, CreatesPolygonsInInputOrder) {
    const auto input = triangleData();
    const TriangleWorld world(input);
    ASSERT_EQ(world.polys().size(), input.data.size());
    for (std::size_t polygon = 0; polygon < input.data.size(); ++polygon) {
        const auto& vertices = world.polys()[polygon].points();
        ASSERT_EQ(vertices.size(), input.data[polygon].size());
        for (std::size_t vertex = 0; vertex < vertices.size(); ++vertex) {
            EXPECT_DOUBLE_EQ(vertices[vertex].x(), input.data[polygon][vertex].x());
            EXPECT_DOUBLE_EQ(vertices[vertex].y(), input.data[polygon][vertex].y());
            EXPECT_DOUBLE_EQ(vertices[vertex].z(), input.data[polygon][vertex].z());
        }
    }
}

TEST(PolyWorld, OwnsVerticesIndependentlyOfInput) {
    auto input = triangleData();
    const TriangleWorld world(input);
    input.data[0][1] = geometry::point_t(100, 100, 100);
    input.data.clear();
    ASSERT_EQ(world.polys().size(), 2U);
    EXPECT_DOUBLE_EQ(world.polys()[0].points()[1].x(), 2);
    EXPECT_DOUBLE_EQ(world.polys()[1].points()[0].z(), 5);
}

TEST(PolyWorld, AcceptsEmptyWorld) {
    const TriangleWorld world(triangles::InitPolysData{});
    EXPECT_TRUE(world.polys().empty());
}

TEST(PolyWorld, DelegatesPolygonValidationToGeometry) {
    const triangles::InitPolysData invalidInputs[] = {
        {{{{0, 0, 0}, {1, 0, 0}}}},
        {{{{0, 0, 0}, {0, 0, 0}, {0, 1, 0}}}},
        {{{{0, 0, 0}, {1, 0, 0}, {2, 0, 0}}}},
        {{{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 1}}}},
        {{{{0, 0, 0}, {2, 2, 0}, {0, 2, 0}, {2, 0, 0}}}}};
    for (const auto& input : invalidInputs) {
        EXPECT_THROW((TriangleWorld(input)), std::invalid_argument);
    }
}

TEST(PolyWorld, AcceptsPolygonsWithDifferentVertexCounts) {
    const triangles::InitPolysData input{
        {{{0, 0, 0}, {2, 0, 0}, {0, 2, 0}},
         {{0, 0, 1}, {2, 0, 1}, {2, 2, 1}, {0, 2, 1}},
         {{0, 0, 2}, {2, 0, 2}, {3, 1, 2}, {1, 3, 2}, {-1, 1, 2}}}};
    const TriangleWorld world(input);
    ASSERT_EQ(world.polys().size(), 3U);
    EXPECT_EQ(world.polys()[0].numOfPoints(), 3U);
    EXPECT_EQ(world.polys()[1].numOfPoints(), 4U);
    EXPECT_EQ(world.polys()[2].numOfPoints(), 5U);
    EXPECT_DOUBLE_EQ(world.polys()[0].area(), 2);
    EXPECT_DOUBLE_EQ(world.polys()[1].area(), 4);
}

TEST(PolyWorld, CopyAssignmentReconstructsIndependentPolygons) {
    const TriangleWorld original(triangleData());
    TriangleWorld copy(triangles::InitPolysData{});
    copy = original;
    ASSERT_EQ(copy.polys().size(), original.polys().size());
    EXPECT_NE(&copy.polys()[0], &original.polys()[0]);
    EXPECT_DOUBLE_EQ(copy.polys()[1].point(0).z(), 5);
}

TEST(PolyWorld, SupportsPolygonsWithFourVertices) {
    const triangles::InitPolysData input{
        {{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}}}};
    const triangles::PolyWorld<3> world(input);
    ASSERT_EQ(world.polys().size(), 1U);
    ASSERT_EQ(world.polys()[0].points().size(), 4U);
    EXPECT_DOUBLE_EQ(world.polys()[0].points()[3].y(), 1);
}

TEST(PolyWorld, ParsesInputAndPassesWorldToProcessor) {
    auto tokens = lexer::Lexer("3 0 0 0 2 0 0 0 2 0").tokenize();
    const auto input = triangles::Parser{}.parse(tokens);
    TriangleWorld world(input);
    const TriangleProcessor processor(std::move(world));
    ASSERT_EQ(processor.world().polys().size(), 1U);
    EXPECT_DOUBLE_EQ(processor.world().polys()[0].points()[1].x(), 2);
    EXPECT_DOUBLE_EQ(processor.world().polys()[0].points()[2].y(), 2);
}

TEST(PolyProcessor, CopiesLvalueWorldWithoutSharingVertices) {
    TriangleWorld world(triangleData());
    const TriangleProcessor processor(world);
    ASSERT_EQ(processor.world().polys().size(), world.polys().size());
    EXPECT_NE(&processor.world().polys()[0].points()[0],
              &world.polys()[0].points()[0]);
}

} // namespace
