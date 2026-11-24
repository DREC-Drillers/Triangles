#pragma once

#include "polygonProcessor/polyWorldCalculator/intersections/intersectionData.hpp"

namespace general_functions {
class Bvh;
}

namespace triangles {

class PolyWorldCalculator {
public:
    IntersectionData calculateIntersections(const PolyWorld& world) const;
    IntersectionData calculateIntersections(const PolyWorld&& world) const = delete;

private:
    general_functions::Bvh prepareBvh(const PolyWorld& world) const;
};

} // namespace triangles
