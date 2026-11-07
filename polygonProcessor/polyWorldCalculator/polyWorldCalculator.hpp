#pragma once

#include "polygonProcessor/polyWorldCalculator/intersections/intersectionData.hpp"

namespace general_functions {
class Bvh;
}

namespace triangles {

class PolyWorldCalculator {
public:
    IntersectionData calculateIntersections(const PolyWorld<3>& world) const;
    IntersectionData
    calculateIntersections(const PolyWorld<3>&& world) const = delete;

private:
    general_functions::Bvh prepareBvh(const PolyWorld<3>& world) const;
};

} // namespace triangles
