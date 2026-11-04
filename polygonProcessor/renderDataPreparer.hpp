#pragma once

#include <array>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include "scene/sceneObject.hpp"
#include "intersectionData.hpp"

namespace triangles {

struct RenderStyle {
    std::array<float, 4> polyColor{0.7f, 0.7f, 0.7f, 1.0f};
    std::array<float, 4> intersectingPolyColor{1.0f, 0.0f, 0.0f, 1.0f};
};

template <std::size_t dimWorld>
class RenderDataPreparer {
    static_assert(dimWorld == 3, "Visualizer supports only 3D coordinates");

public:
    std::vector<visualizer::SceneObject>
    prepare(const IntersectionData& intersections,
            const RenderStyle& style = {}) const {
        std::vector<visualizer::SceneObject> objects;
        objects.reserve(intersections.nonIntersectingPolys.size() +
                        intersections.intersectingPolys.size());

        const auto append = [&](const std::vector<const geometry::polygon_t<3>*>& polygons,
                                const std::array<float, 4>& color) {
            for (const auto* polygon : polygons) {
                if (polygon == nullptr) {
                    throw std::invalid_argument("RenderDataPreparer: null polygon");
                }
                objects.push_back(prepareTriangle(*polygon, color));
            }
        };

        append(intersections.nonIntersectingPolys, style.polyColor);
        append(intersections.intersectingPolys, style.intersectingPolyColor);
        return objects;
    }

private:
    static visualizer::SceneObject
    prepareTriangle(const geometry::polygon_t<3>& polygon, const std::array<float, 4>& color) {
        const auto& points = polygon.points();
        if (points.size() != 3) {
            throw std::invalid_argument("RenderDataPreparer: expected exactly 3 vertices");
        }

        visualizer::SceneObject object;
        object.geometry.topology = visualizer::Topology::Triangles;
        object.material.color = color;
        object.material.lit = true;
        object.geometry.vertices.reserve(3);

        const auto n = polygon.unitNormal();
        const std::array<float, 3> normal{
            static_cast<float>(n.x()),
            static_cast<float>(n.y()),
            static_cast<float>(n.z())};

        for (const auto& point : points) {
            object.geometry.vertices.push_back({
                {static_cast<float>(point.x()),
                 static_cast<float>(point.y()),
                 static_cast<float>(point.z())},
                normal});
        }
        return object;
    }
};

} // namespace triangles
