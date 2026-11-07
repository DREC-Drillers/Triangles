#pragma once

import geometry;

namespace triangles {

class PolyCalculator {
public:
    bool intersects(const geometry::polygon_t<3>& first,
                    const geometry::polygon_t<3>& second) const;

    bool intersects(const geometry::triangle_t<3>& first,
                    const geometry::triangle_t<3>& second) const;
};

} // namespace triangles
