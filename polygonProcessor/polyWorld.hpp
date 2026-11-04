#pragma once

#include <array>
#include <cstddef>
#include <utility>
#include <vector>

#include "initPolysData.hpp"

import geometry;

namespace triangles {

template <std::size_t dimWorld>
class PolyWorld {
    static_assert(dimWorld == 3,
                  "InitPolysData currently supports only 3D coordinates");

private:
    std::vector<geometry::polygon_t<3>> polys_;

public:
    explicit PolyWorld(const InitPolysData& initData) {
        polys_.reserve(initData.data.size());
        for (const auto& vertices : initData.data) {
            std::vector<geometry::point_t<3>> points;
            points.reserve(vertices.size());
            for (const auto& vertex : vertices) {
                points.emplace_back(
                    std::array<double, 3>{vertex.x(), vertex.y(), vertex.z()});
            }
            polys_.emplace_back(std::move(points));
        }
    }

    const std::vector<geometry::polygon_t<3>>& polys() const { return polys_; }
};

} // namespace triangles
