#include "triangleBounds.hpp"

#include <algorithm>

namespace triangles {

Aabb triangleBounds(const geometry::triangle_t<3>& triangle) {
    const auto& a = triangle.a();
    const auto& b = triangle.b();
    const auto& c = triangle.c();

    return Aabb({std::min({a.x(), b.x(), c.x()}),
                 std::min({a.y(), b.y(), c.y()}),
                 std::min({a.z(), b.z(), c.z()})},
                {std::max({a.x(), b.x(), c.x()}),
                 std::max({a.y(), b.y(), c.y()}),
                 std::max({a.z(), b.z(), c.z()})});
}

} // namespace triangles
