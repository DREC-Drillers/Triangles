#pragma once

#include <cstddef>

#include "polyWorld.hpp"
#include "polyWorldCalculator/intersections/intersectionData.hpp"
#include "polyWorldCalculator/polyWorldCalculator.hpp"

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
        PolyWorldCalculator calculator;
        return calculateIntersections(calculator);
    }

    template <typename Calculator>
    IntersectionData calculateIntersections(Calculator& calculator) const {
        return calculator.calculateIntersections(world_);
    }
};

} // namespace triangles
