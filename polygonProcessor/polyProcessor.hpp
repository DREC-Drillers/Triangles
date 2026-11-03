#pragma once

#include <cstddef>

#include "intersectionData.hpp"
#include "polyCalculator.hpp"
#include "polyWorld.hpp"

namespace triangles {

template <std::size_t dimWorld>
class PolyProcessor {
private:
    const PolyWorld<dimWorld>& world_;

public:
    explicit PolyProcessor(const PolyWorld<dimWorld>& world) : world_(world) {}
    PolyProcessor(PolyWorld<dimWorld>&&) = delete;
    PolyProcessor(const PolyWorld<dimWorld>&&) = delete;

    const PolyWorld<dimWorld>& world() const { return world_; }

    IntersectionData calculateIntersections() const {
        PolyCalculator calculator;
        return calculateIntersections(calculator);
    }

    template <typename Calculator>
    IntersectionData calculateIntersections(Calculator& calculator) const {
        return calculator.calculateIntersections(world_.polys());
    }
};

} // namespace triangles
