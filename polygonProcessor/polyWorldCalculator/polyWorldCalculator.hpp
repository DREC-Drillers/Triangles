#pragma once

#include "polygonProcessor/polyWorldCalculator/intersections/intersectionData.hpp"

namespace triangles {

class PolyWorldCalculator {
public:
    static IntersectionData calculateIntersections(const PolyWorld& world);
    static IntersectionData calculateIntersections(const PolyWorld&& world) = delete;
};

} // namespace triangles
