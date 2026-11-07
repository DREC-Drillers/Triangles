#pragma once

#include <array>
#include <cstddef>
#include <memory>
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
    std::vector<std::unique_ptr<geometry::polygon_t<3>>> polys_;

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

            if (points.size() == 3) {
                polys_.push_back(std::make_unique<geometry::triangle_t<3>>(
                    points[0], points[1], points[2]));
            } else {
                polys_.push_back(
                    std::make_unique<geometry::polygon_t<3>>(std::move(points)));
            }
        }
    }

    ~PolyWorld() = default;

    PolyWorld(const PolyWorld&) = delete;
    PolyWorld& operator=(const PolyWorld&) = delete;

    PolyWorld(PolyWorld&&) noexcept = default;
    PolyWorld& operator=(PolyWorld&&) noexcept = default;

    const std::vector<std::unique_ptr<geometry::polygon_t<3>>>& polys() const {
        return polys_;
    }
};

} // namespace triangles
