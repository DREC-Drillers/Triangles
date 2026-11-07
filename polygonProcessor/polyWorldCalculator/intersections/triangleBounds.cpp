#include "triangleBounds.hpp"

#include <algorithm>

namespace triangles {

geometry::axisAlignedBoundingBox_t
triangleBounds(const geometry::triangle_t<3>& triangle) {
    const auto& a = triangle.a();
    const auto& b = triangle.b();
    const auto& c = triangle.c();

    return geometry::axisAlignedBoundingBox_t(
        geometry::point_t<3>({std::min({a.x(), b.x(), c.x()}),
                              std::min({a.y(), b.y(), c.y()}),
                              std::min({a.z(), b.z(), c.z()})}),
        geometry::point_t<3>({std::max({a.x(), b.x(), c.x()}),
                              std::max({a.y(), b.y(), c.y()}),
                              std::max({a.z(), b.z(), c.z()})}));
}

} // namespace triangles
