#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "initPolysData.hpp"

import geometry;

namespace triangles {

class PolyWorld {
private:
    std::vector<std::unique_ptr<geometry::polygon_t<3>>> polys_;

public:
    explicit PolyWorld(const InitPolysData& initData) {
        polys_.reserve(initData.data.size());

        for (const auto& vertices : initData.data) {
            if (vertices.size() == 3) {
                polys_.push_back(std::make_unique<geometry::triangle_t<3>>(
                    vertices[0], vertices[1], vertices[2]));
            } else {
                polys_.push_back(std::make_unique<geometry::polygon_t<3>>(vertices));
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
