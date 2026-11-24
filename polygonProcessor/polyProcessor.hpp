#pragma once

#include <cstddef>

#include "polyWorld.hpp"
#include "polyWorldCalculator/intersections/intersectionData.hpp"
#include "polyWorldCalculator/polyWorldCalculator.hpp"

namespace triangles {

class PolyProcessor {
private:
    const PolyWorld& world_;

public:
    explicit PolyProcessor(const PolyWorld& world) : world_(world) {}
    PolyProcessor(PolyWorld&&) = delete;
    PolyProcessor(const PolyWorld&&) = delete;

    const PolyWorld& world() const { return world_; }

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
