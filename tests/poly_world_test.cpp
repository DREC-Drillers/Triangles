#include <stdexcept>
#include <utility>

#include <gtest/gtest.h>

#include "lexer/lexer.h"
#include "parser/parser.hpp"
#include "polygonProcessor/polyProcessor.hpp"

namespace {

using TriangleWorld = triangles::PolyWorld<3, 3>;
using TriangleProcessor = triangles::PolyProcessor<3, 3>;

triangles::InitPolysData triangleData() {
    return {{{{0, 0, 0}, {2, 0, 0}, {0, 2, 0}}, {{0, 0, 5}, {3, 0, 5}, {0, 3, 5}}}};
}

TEST(PolyWorld, CreatesPolygonsInInputOrder) {
    const auto input = triangleData();
    const TriangleWorld world(input);
    ASSERT_EQ(world.polys().size(), input.data.size());
    for (std::size_t polygon = 0; polygon < input.data.size(); ++polygon) {
        const auto& vertices = world.polys()[polygon].vertices();
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
    input.data[0][1] = triangles::Point(100, 100, 100);
    input.data.clear();
    ASSERT_EQ(world.polys().size(), 2U);
    EXPECT_DOUBLE_EQ(world.polys()[0].vertices()[1].x(), 2);
    EXPECT_DOUBLE_EQ(world.polys()[1].vertices()[0].z(), 5);
}

TEST(PolyWorld, AcceptsEmptyWorld) {
    const TriangleWorld world(triangles::InitPolysData{});
    EXPECT_TRUE(world.polys().empty());
}

TEST(PolyWorld, RejectsWrongVertexCount) {
    for (const std::size_t count : {0U, 1U, 2U, 4U}) {
        auto input = triangleData();
        input.data[1].resize(count);
        EXPECT_THROW((TriangleWorld(input)), std::invalid_argument);
    }
}

TEST(PolyWorld, SupportsPolygonsWithFourVertices) {
    const triangles::InitPolysData input{
        {{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}}}};
    const triangles::PolyWorld<3, 4> world(input);
    ASSERT_EQ(world.polys().size(), 1U);
    ASSERT_EQ(world.polys()[0].vertices().size(), 4U);
    EXPECT_DOUBLE_EQ(world.polys()[0].vertices()[3].y(), 1);
}

TEST(PolyWorld, ParsesInputAndPassesWorldToProcessor) {
    auto tokens = lexer::Lexer("1 0 0 0 2 0 0 0 2 0").tokenize();
    const auto input = triangles::Parser{}.parse(tokens);
    TriangleWorld world(input);
    const TriangleProcessor processor(std::move(world));
    ASSERT_EQ(processor.world().polys().size(), 1U);
    EXPECT_DOUBLE_EQ(processor.world().polys()[0].vertices()[1].x(), 2);
    EXPECT_DOUBLE_EQ(processor.world().polys()[0].vertices()[2].y(), 2);
}

TEST(PolyProcessor, CanAlsoConstructWorldDirectlyFromInput) {
    const TriangleProcessor processor(triangleData());
    EXPECT_EQ(processor.world().polys().size(), 2U);
}

TEST(PolyProcessor, CopiesLvalueWorldWithoutSharingVertices) {
    TriangleWorld world(triangleData());
    const TriangleProcessor processor(world);
    ASSERT_EQ(processor.world().polys().size(), world.polys().size());
    EXPECT_NE(&processor.world().polys()[0].vertices()[0],
              &world.polys()[0].vertices()[0]);
}

} // namespace
