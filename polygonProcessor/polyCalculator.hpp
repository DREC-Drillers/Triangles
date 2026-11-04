#pragma once

#include "intersectionData.hpp"
#include "polyWorld.hpp"

namespace triangles {

class PolyCalculator {
public:
    IntersectionData calculateIntersections(const PolyWorld<3>& world);
    IntersectionData calculateIntersections(const PolyWorld<3>&& world) = delete;
};

} // namespace triangles
