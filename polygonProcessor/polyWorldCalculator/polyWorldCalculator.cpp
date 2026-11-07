#include "polyWorldCalculator.hpp"

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

#include "intersections/triangleBounds.hpp"
#include "polyCalculator.hpp"
#include "spetial/bvh.hpp"

namespace triangles {

using geometry::axisAlignedBoundingBox_t;
using general_functions::Bvh;

struct PolyWorldCalculator::PreparedIntersectionSearch {
    std::vector<geometry::triangle_t<3>> triangles;
    Bvh index;
};

PolyWorldCalculator::PreparedIntersectionSearch
PolyWorldCalculator::prepareIntersectionSearch(const PolyWorld<3>& world) const {
    std::vector<geometry::triangle_t<3>> triangles;
    triangles.reserve(world.polys().size());

    for (const auto& polygon : world.polys()) {
        if (polygon.numOfPoints() != 3) {
            throw std::invalid_argument("Only triangles are supported; other "
                                        "polygons are not supported yet");
        }
        triangles.emplace_back(polygon.point(0), polygon.point(1), polygon.point(2));
    }

    std::vector<axisAlignedBoundingBox_t> bounds;
    bounds.reserve(triangles.size());
    for (const auto& triangle : triangles) {
        bounds.push_back(triangleBounds(triangle));
    }
    return {std::move(triangles), Bvh(std::move(bounds))};
}

IntersectionData
PolyWorldCalculator::calculateIntersections(const PolyWorld<3>& world) const {
    IntersectionData intersections{world, {}, {}};

    const auto& polys = world.polys();
    const auto prepared = prepareIntersectionSearch(world);
    const auto& triangles = prepared.triangles;

    const PolyCalculator calculator;
    std::vector<bool> intersects(triangles.size(), false);

    for (std::size_t first = 0; first < triangles.size(); ++first) {
        const auto& triangle = triangles[first];

        const auto checkPair = [&](std::size_t second) {
            if (second < first &&
                calculator.intersects(triangle, triangles.at(second))) {
                intersects[first] = true;
                intersects[second] = true;
            }
            return true;
        };

        prepared.index.query(prepared.index.objectBounds(first), checkPair);
    }

    for (std::size_t i = 0; i < polys.size(); ++i) {
        auto& collection = intersects[i] ? intersections.intersectingPolys
                                         : intersections.nonIntersectingPolys;
        collection.push_back(&polys[i]);
    }

    return intersections;
}

} // namespace triangles
