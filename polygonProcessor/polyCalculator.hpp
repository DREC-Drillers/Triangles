#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "intersectionData.hpp"

namespace triangles {

template <std::size_t dimPoly>
class Poly;

template <std::size_t dimPoly>
class PolyCalculator {
public:
    // Collect all intersecting pairs and their intersection geometry.
    IntersectionData calculateIntersections(const std::vector<Poly<dimPoly>>& polys);

private:
    // std::nullopt means the pair does not intersect.
    std::optional<PolyIntersection> intersectsPoly(const Poly<dimPoly>& first,
                                                   const Poly<dimPoly>& second);
};

} // namespace triangles
