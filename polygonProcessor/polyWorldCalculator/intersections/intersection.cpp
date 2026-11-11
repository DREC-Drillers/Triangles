#include "intersection.hpp"

namespace triangles {

RealTriangleForm determineRealTriangleForm(const geometry::triangle_t<3>& triangle) {
    const auto& a = triangle.a();
    const auto& b = triangle.b();
    const auto& c = triangle.c();

    if (a == b && b == c) {
        return RealTriangleForm::POINT;
    }
    if (a == b || b == c || a == c) {
        return RealTriangleForm::SEGMENT;
    }
    return RealTriangleForm::TRIANGLE;
}

} // namespace triangles
