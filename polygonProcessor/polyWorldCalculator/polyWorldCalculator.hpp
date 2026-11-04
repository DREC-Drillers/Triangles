#pragma once

#include "polygonProcessor/polyWorldCalculator/intersections/intersectionData.hpp"

namespace triangles {

class PolyWorldCalculator {
public:
    IntersectionData calculateIntersections(const PolyWorld<3>& world) const;
    IntersectionData
    calculateIntersections(const PolyWorld<3>&& world) const = delete;
};

} // namespace triangles
