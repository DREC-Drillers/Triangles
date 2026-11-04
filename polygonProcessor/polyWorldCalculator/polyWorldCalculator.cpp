#include "polyWorldCalculator.hpp"

#include <cstddef>
#include <stdexcept>
#include <vector>

#include "polyCalculator.hpp"

namespace triangles {

IntersectionData
PolyWorldCalculator::calculateIntersections(const PolyWorld<3>& world) const {
    IntersectionData intersections{world, {}, {}};

    const auto& polys = world.polys();
    std::vector<geometry::triangle_t<3>> triangles;
    triangles.reserve(polys.size());

    for (const auto& polygon : polys) {
        if (polygon.numOfPoints() != 3) {
            throw std::invalid_argument("Only triangles are supported; other "
                                        "polygons are not supported yet");
        }
        triangles.emplace_back(polygon.point(0), polygon.point(1), polygon.point(2));
    }

    const PolyCalculator calculator;
    std::vector<bool> intersects(triangles.size(), false);
    for (std::size_t first = 0; first < triangles.size(); ++first) {
        const auto& triangle = triangles[first];

        for (std::size_t second = 0; second < first; ++second) {
            const auto& other = triangles[second];
            if (calculator.intersects(triangle, other)) {
                intersects[first] = true;
                intersects[second] = true;
            }
        }
    }

    for (std::size_t i = 0; i < polys.size(); ++i) {
        auto& collection = intersects[i] ? intersections.intersectingPolys
                                         : intersections.nonIntersectingPolys;
        collection.push_back(&polys[i]);
    }

    return intersections;
}

} // namespace triangles
