#include "parser.hpp"

#include <array>
#include <string>
#include <utility>

#include "file.h"
#include "lexer/lexer.h"
#include "lexer/token_stream.h"
#include "settings.hpp"
#include "spatial/geometry/geomConst.hpp"

namespace triangles {

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
        const auto polygonContext = [polygonNumber] {
            return " (polygon " + std::to_string(polygonNumber) + ")";
        };
        const lexer::Token& countToken = tokens.peekToken();
        if (countToken.type != lexer::TokenType::INT) {
            tokens.failAtToken(
                countToken, "Expected an integer vertex count" + polygonContext());
        }
        const auto count = tokens.expectInt({});
        if (count < 3 || count >= settings::kVertexCountUpperBound) {
            tokens.failAtToken(countToken,
                               "Vertex count must be 3 <= M < " +
                                   std::to_string(settings::kVertexCountUpperBound) +
                                   polygonContext());
        }

        InitPolysData::PolygonPoints vertices;
        vertices.reserve(static_cast<std::size_t>(count));
        for (std::size_t vertex = 0; vertex < static_cast<std::size_t>(count);
             ++vertex) {
            const auto readCoordinate = [&tokens, polygonNumber, vertex](char axis) {
                const auto& token = tokens.peekToken();
                if (token.type != lexer::TokenType::INT &&
                    token.type != lexer::TokenType::DOUBLE) {
                    tokens.failAtToken(
                        token,
                        std::string("Expected ") + axis + " coordinate (polygon " +
                            std::to_string(polygonNumber) + ", vertex " +
                            std::to_string(vertex + 1) + ")");
                }
                return tokens.expectDouble({});
            };
            const double x = readCoordinate('x');
            const double y = readCoordinate('y');
            const double z = readCoordinate('z');
            vertices.emplace_back(std::array<double, geometry::defaultDim>{x, y, z});
        }
        result.data.push_back(std::move(vertices));
    }
    return result;
}

} // namespace triangles
