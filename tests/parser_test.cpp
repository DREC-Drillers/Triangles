#include <array>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>

#include "lexer/lexer.h"
#include "parser/parser.hpp"

namespace {

static_assert(std::is_same_v<triangles::InitPolysData::PolygonPoints::value_type,
                             geometry::point_t>);

triangles::InitPolysData parse(const std::string& source) {
    auto tokens = lexer::Lexer(source, "scene.txt").tokenize();
    return triangles::Parser{}.parse(tokens);
}

TEST(Parser, ReadsIntegerAndFractionalCoordinates) {
    const auto result = parse("3\n0 -2 3\n0.5 -1.25 +2.5\n4 5 6");
    ASSERT_EQ(result.data.size(), 1U);
    ASSERT_EQ(result.data[0].size(), 3U);
    EXPECT_DOUBLE_EQ(result.data[0][0].x(), 0);
    EXPECT_DOUBLE_EQ(result.data[0][0].y(), -2);
    EXPECT_DOUBLE_EQ(result.data[0][0].z(), 3);
    EXPECT_DOUBLE_EQ(result.data[0][1].x(), 0.5);
    EXPECT_DOUBLE_EQ(result.data[0][1].y(), -1.25);
    EXPECT_DOUBLE_EQ(result.data[0][1].z(), 2.5);
    EXPECT_DOUBLE_EQ(result.data[0][2].x(), 4);
    EXPECT_DOUBLE_EQ(result.data[0][2].y(), 5);
    EXPECT_DOUBLE_EQ(result.data[0][2].z(), 6);
}

TEST(Parser, ReadsPolygonsWithDifferentVertexCountsUntilEnd) {
    auto tokens = lexer::Lexer("3 0 0 0 2 0 0 0 2 0\n\n"
                               "4 0 0 1 2 0 1 2 2 1 0 2 1\n"
                               "5 0 0 2 2 0 2 3 1 2 1 3 2 -1 1 2 \t\n")
                      .tokenize();
    const auto result = triangles::Parser{}.parse(tokens);
    ASSERT_EQ(result.data.size(), 3U);
    ASSERT_EQ(result.data[0].size(), 3U);
    ASSERT_EQ(result.data[1].size(), 4U);
    ASSERT_EQ(result.data[2].size(), 5U);
    EXPECT_DOUBLE_EQ(result.data[0][1].x(), 2);
    EXPECT_DOUBLE_EQ(result.data[1][3].y(), 2);
    EXPECT_DOUBLE_EQ(result.data[1][3].z(), 1);
    EXPECT_DOUBLE_EQ(result.data[2][4].x(), -1);
    EXPECT_DOUBLE_EQ(result.data[2][4].y(), 1);
    EXPECT_DOUBLE_EQ(result.data[2][4].z(), 2);
    EXPECT_TRUE(tokens.atEnd());
}

TEST(Parser, ReadsAdjacentPolygonsWithSameVertexCount) {
    const auto result = parse("3 0 0 0 2 0 0 0 2 0 3 0 0 5 2 0 5 0 2 5");
    ASSERT_EQ(result.data.size(), 2U);
    ASSERT_EQ(result.data[0].size(), 3U);
    ASSERT_EQ(result.data[1].size(), 3U);
    EXPECT_DOUBLE_EQ(result.data[0][2].z(), 0);
    EXPECT_DOUBLE_EQ(result.data[1][0].z(), 5);
    EXPECT_DOUBLE_EQ(result.data[1][2].z(), 5);
}

TEST(Parser, ProducesGeneralFunctionsPoints) {
    const auto result = parse("3 0 0 0 3 0 0 0 4 0");
    ASSERT_EQ(result.data.size(), 1U);
    ASSERT_EQ(result.data[0].size(), 3U);
    EXPECT_DOUBLE_EQ(result.data[0][0].distance(result.data[0][1]), 3);
    EXPECT_DOUBLE_EQ(result.data[0][1].distance(result.data[0][2]), 5);
}

TEST(Parser, PreservesConcavePolygonVertexOrder) {
    const auto result = parse("5 0 0 0 2 0 0 1 1 0 2 2 0 0 2 0");
    ASSERT_EQ(result.data.size(), 1U);
    ASSERT_EQ(result.data[0].size(), 5U);
    EXPECT_DOUBLE_EQ(result.data[0][1].x(), 2);
    EXPECT_DOUBLE_EQ(result.data[0][2].x(), 1);
    EXPECT_DOUBLE_EQ(result.data[0][2].y(), 1);
    EXPECT_DOUBLE_EQ(result.data[0][3].x(), 2);
    EXPECT_DOUBLE_EQ(result.data[0][4].y(), 2);
}

class InvalidParserInput : public testing::TestWithParam<const char*> {};

TEST_P(InvalidParserInput, RejectsInvalidScene) {
    EXPECT_THROW(parse(GetParam()), std::runtime_error);
}

INSTANTIATE_TEST_SUITE_P(
    MalformedScenes, InvalidParserInput,
    testing::Values("", " \t\n", "0", "-1", "1", "2", "1000000", "1000001",
                    "9223372036854775807", "3.0", "name", "3", "3 0 0 0 1 0 0 0 1",
                    "4 0 0 0 1 0 0 0 1 0", "3 name 0 0 1 0 0 0 1 0",
                    "3 0 0 0 1 0 0 0 1 0 extra", "3 0 0 0 1 0 0 0 1 0 1",
                    "3 0 0 0 1 0 0 0 1 0 4"));

TEST(Parser, ReportsFilePositionAndVertexForMissingCoordinate) {
    try {
        (void)parse("3\n0 0 0\n1 0 0\n0 1");
        FAIL() << "Expected missing z coordinate to fail";
    } catch (const std::runtime_error& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find("scene.txt:4:4:"), std::string::npos);
        EXPECT_NE(message.find("Expected z coordinate (polygon 1, vertex 3)"),
                  std::string::npos);
    }
}

TEST(Parser, ReportsInvalidVertexCountInSecondPolygon) {
    try {
        (void)parse("3 0 0 0 1 0 0 0 1 0\n2");
        FAIL() << "Expected invalid vertex count to fail";
    } catch (const std::runtime_error& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find("scene.txt:2:1:"), std::string::npos);
        EXPECT_NE(message.find("Vertex count must be 3 <= M < 1000000 (polygon 2)"),
                  std::string::npos);
    }
}

TEST(Parser, ReportsMissingCoordinateInSecondPolygon) {
    try {
        (void)parse("3 0 0 0 1 0 0 0 1 0\n4 0 0 1 1");
        FAIL() << "Expected incomplete polygon to fail";
    } catch (const std::runtime_error& error) {
        EXPECT_NE(std::string(error.what())
                      .find("Expected y coordinate (polygon 2, vertex 2)"),
                  std::string::npos);
    }
}

TEST(Parser, ReadsAllExampleFiles) {
    const std::array<std::size_t, 6> polygonCounts{1, 2, 2, 2, 2, 3};
    for (std::size_t index = 0; index < polygonCounts.size(); ++index) {
        const auto path = std::filesystem::path(TRIANGLES_SOURCE_DIR) / "inputs" /
                          ("input_" + std::to_string(index + 1) + ".txt");
        SCOPED_TRACE(path.string());
        const auto result = triangles::Parser{}.parse(path);
        ASSERT_EQ(result.data.size(), polygonCounts[index]);
        for (std::size_t polygon = 0; polygon < result.data.size(); ++polygon) {
            const auto expected = index == 5 ? polygon + 3 : 3;
            EXPECT_EQ(result.data[polygon].size(), expected);
        }
    }
}

TEST(Parser, ReadsSceneCoordinatesFromFile) {
    const auto result = triangles::Parser{}.parse(
        std::filesystem::path(TRIANGLES_SOURCE_DIR) / "inputs/input_1.txt");
    ASSERT_EQ(result.data.size(), 1U);
    ASSERT_EQ(result.data[0].size(), 3U);
    EXPECT_DOUBLE_EQ(result.data[0][1].x(), 2);
    EXPECT_DOUBLE_EQ(result.data[0][2].y(), 2);
}

TEST(Parser, RejectsMissingFile) {
    EXPECT_THROW(
        triangles::Parser{}.parse(std::filesystem::path(TRIANGLES_SOURCE_DIR) /
                                  "tests/nonexistent-directory/scene.txt"),
        std::runtime_error);
}

} // namespace
