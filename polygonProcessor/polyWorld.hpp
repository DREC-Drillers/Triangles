#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include "initPolysData.hpp"

namespace triangles {

template <std::size_t dimPoly>
class Poly {
    static_assert(dimPoly >= 3, "A polygon must have at least three vertices");

private:
    std::array<Point, dimPoly> vertices_;

public:
    explicit Poly(const std::vector<Point>& vertices) {
        if (vertices.size() != dimPoly) {
            throw std::invalid_argument("Expected " + std::to_string(dimPoly) +
                                        " polygon vertices, got " +
                                        std::to_string(vertices.size()));
        }
        std::copy(vertices.begin(), vertices.end(), vertices_.begin());
    }

    const std::array<Point, dimPoly>& vertices() const { return vertices_; }
};

template <std::size_t dimWorld, std::size_t dimPoly>
class PolyWorld {
    static_assert(dimWorld == 3,
                  "InitPolysData currently supports only 3D coordinates");

private:
    std::vector<Poly<dimPoly>> polys_;

public:
    explicit PolyWorld(const InitPolysData& initData) {
        polys_.reserve(initData.data.size());
        for (const auto& vertices : initData.data) {
            polys_.emplace_back(vertices);
        }
    }

    const std::vector<Poly<dimPoly>>& polys() const { return polys_; }
};

} // namespace triangles
