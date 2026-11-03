#pragma once

#include <cstddef>
#include <vector>

#include "initPolysData.hpp"

namespace triangles {

template <std::size_t dimWorld>
class PolyWorld {
    static_assert(dimWorld == 3,
                  "InitPolysData currently supports only 3D coordinates");

private:
    std::vector<geometry::polygon_t> polys_;

public:
    explicit PolyWorld(const InitPolysData& initData) {
        polys_.reserve(initData.data.size());
        for (const auto& vertices : initData.data) {
            polys_.emplace_back(vertices);
        }
    }

    PolyWorld(const PolyWorld&) = default;
    PolyWorld(PolyWorld&&) = default;
    PolyWorld& operator=(PolyWorld&&) = default;

    PolyWorld& operator=(const PolyWorld&) = default;

    const std::vector<geometry::polygon_t>& polys() const { return polys_; }
};

} // namespace triangles
