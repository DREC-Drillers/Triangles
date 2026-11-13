#include "parser.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <utility>

#include "file.h"
#include "lexer/lexer.h"
#include "lexer/token_stream.h"

namespace triangles {

constexpr std::size_t MAX_POLYGON_VERTICES = 1'000'000;

InitPolysData Parser::parse(const std::filesystem::path& path) const {
    lexer::Lexer input(generalFunctions::readFile(path), path);
    auto tokens = input.tokenize();
    return parse(tokens);
}

InitPolysData Parser::parse(lexer::TokenStream& tokens) const {
    InitPolysData result;
    if (tokens.atEnd()) {
        tokens.failAtToken(tokens.peekToken(), "Expected at least one polygon");
    }

    while (!tokens.atEnd()) {
        const auto polygonNumber = result.data.size() + 1;
        const std::string polygonContext =
            " (polygon " + std::to_string(polygonNumber) + ")";
        const lexer::Token& countToken = tokens.peekToken();
        const auto count =
            tokens.expectInt("Expected an integer vertex count" + polygonContext);
        if (count < 3 || count >= static_cast<std::int64_t>(MAX_POLYGON_VERTICES)) {
            tokens.failAtToken(countToken,
                               "Vertex count must be 3 <= M < 1000000" +
                                   polygonContext);
        }

        InitPolysData::PolygonPoints vertices;
        vertices.reserve(static_cast<std::size_t>(count));
        for (std::size_t vertex = 0; vertex < static_cast<std::size_t>(count);
             ++vertex) {
            const std::string context = " (polygon " +
                                        std::to_string(polygonNumber) + ", vertex " +
                                        std::to_string(vertex + 1) + ")";
            const double x = tokens.expectDouble("Expected x coordinate" + context);
            const double y = tokens.expectDouble("Expected y coordinate" + context);
            const double z = tokens.expectDouble("Expected z coordinate" + context);
            vertices.emplace_back(std::array<double, 3>{x, y, z});
        }
        result.data.push_back(std::move(vertices));
    }
    return result;
}

} // namespace triangles
