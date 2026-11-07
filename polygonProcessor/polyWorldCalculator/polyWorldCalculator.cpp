#include "polyWorldCalculator.hpp"

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

#include "intersections/triangleBounds.hpp"
#include "intersections/triangleIntersection.hpp"
#include "spetial/bvh.hpp"

namespace triangles {

using geometry::axisAlignedBoundingBox_t;
using general_functions::Bvh;

namespace {

bool intersectsTriangles(const geometry::triangle_t<3>& first,
                         const geometry::triangle_t<3>& second) {
    if (!triangleBounds(first).overlaps(triangleBounds(second))) {
        return false;
    }
    return doTrianglesIntersect(first, second);
}

} // namespace

struct PolyWorldCalculator::PreparedIntersectionSearch {
    std::vector<const geometry::triangle_t<3>*> triangles;
    Bvh index;
};

PolyWorldCalculator::PreparedIntersectionSearch
PolyWorldCalculator::prepareIntersectionSearch(const PolyWorld<3>& world) const {
    std::vector<const geometry::triangle_t<3>*> triangles;
    triangles.reserve(world.polys().size());

    for (const auto& polygon : world.polys()) {
        const auto* triangle =
            dynamic_cast<const geometry::triangle_t<3>*>(polygon.get());
        if (triangle == nullptr) {
            throw std::invalid_argument("Only triangles are supported; other "
                                        "polygons are not supported yet");
        }
        triangles.push_back(triangle);
    }

    std::vector<axisAlignedBoundingBox_t> bounds;
    bounds.reserve(triangles.size());
    for (const auto& triangle : triangles) {
        bounds.push_back(triangleBounds(*triangle));
    }
    return {std::move(triangles), Bvh(std::move(bounds))};
}

IntersectionData
PolyWorldCalculator::calculateIntersections(const PolyWorld<3>& world) const {
    IntersectionData intersections{world, {}, {}};

    const auto& polys = world.polys();
    const auto prepared = prepareIntersectionSearch(world);
    const auto& triangles = prepared.triangles;

    std::vector<bool> intersects(triangles.size(), false);

    for (std::size_t first = 0; first < triangles.size(); ++first) {
        const auto& firstTriangle = *triangles[first];

        const auto checkPair = [&](std::size_t second) {
            if (second < first &&
                intersectsTriangles(firstTriangle, *triangles.at(second))) {
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
        collection.push_back(polys[i].get());
    }

    return intersections;
}

} // namespace triangles
