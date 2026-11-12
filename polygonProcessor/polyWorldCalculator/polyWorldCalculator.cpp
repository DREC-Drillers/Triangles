#include "polyWorldCalculator.hpp"

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

#include "intersections/triangleBounds.hpp"
#include "intersections/triangleIntersection.hpp"
#include "spetial/bvh.hpp"

namespace triangles {

using general_functions::Bvh;
using geometry::axisAlignedBoundingBox_t;

namespace {

const geometry::triangle_t<3>& asTriangle(const geometry::polygon_t<3>& polygon) {
    const auto* triangle = dynamic_cast<const geometry::triangle_t<3>*>(&polygon);
    if (triangle == nullptr) {
        throw std::invalid_argument("Only triangles are supported; other "
                                    "polygons are not supported yet");
    }
    return *triangle;
}

} // namespace

Bvh PolyWorldCalculator::prepareBvh(const PolyWorld& world) const {
    std::vector<axisAlignedBoundingBox_t> bounds;
    bounds.reserve(world.polys().size());
    for (const auto& polygon : world.polys()) {
        bounds.push_back(triangleBounds(asTriangle(*polygon)));
    }
    return Bvh(std::move(bounds));
}

IntersectionData
PolyWorldCalculator::calculateIntersections(const PolyWorld& world) const {
    IntersectionData intersections{world, {}, {}};

    const auto& polys = world.polys();
    const auto bvh = prepareBvh(world);
    std::vector<bool> intersects(polys.size(), false);

    for (std::size_t first = 0; first < polys.size(); ++first) {
        const auto& firstTriangle = asTriangle(*polys[first]);

        const auto checkPair = [&](std::size_t second) {
            if (second >= first) {
                return true;
            }

            const auto& secondTriangle = asTriangle(*polys.at(second));
            if (doIntersect(firstTriangle, secondTriangle)) {
                intersects[first] = true;
                intersects[second] = true;
            }
            return true;
        };

        bvh.query(bvh.objectBounds(first), checkPair);
    }

    for (std::size_t i = 0; i < polys.size(); ++i) {
        auto& collection = intersects[i] ? intersections.intersectingPolys
                                         : intersections.nonIntersectingPolys;
        collection.push_back(polys[i].get());
    }

    return intersections;
}

} // namespace triangles
