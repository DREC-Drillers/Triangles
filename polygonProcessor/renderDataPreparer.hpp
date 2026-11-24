#pragma once

#include <array>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

#include "polygonProcessor/polyWorldCalculator/intersections/intersectionData.hpp"
#include "scene/sceneObject.hpp"
#include "settings.hpp"
#include "spatial/geometry/geomConst.hpp"

namespace triangles {

class RenderDataPreparer {
public:
    static std::vector<visualizer::SceneObject>
    prepare(const IntersectionData& intersections, const RenderStyle& style = {}) {
        std::vector<visualizer::SceneObject> objects;

        const auto append =
            [&](const std::vector<const geometry::polygon_t<geometry::defaultDim>*>&
                    polygons,
                const std::array<float, 4>& color) {
                Batches batches{
                    makeBatch(visualizer::Topology::POINTS, color),
                    makeBatch(visualizer::Topology::LINES, color),
                    makeBatch(visualizer::Topology::TRIANGLES, color),
                };

                for (const auto* polygon : polygons) {
                    if (polygon == nullptr) {
                        throw std::invalid_argument(
                            "RenderDataPreparer: null polygon");
                    }
                    appendPrimitive(batches, *polygon);
                }
                for (auto& batch : batches) {
                    if (!batch.geometry.vertices.empty()) {
                        batch.geometry.freezeVertices();
                        objects.push_back(std::move(batch));
                    }
                }
            };

        append(intersections.nonIntersectingPolys, style.polyColor);
        append(intersections.intersectingPolys, style.intersectingPolyColor);
        return objects;
    }

private:
    using Batches = std::array<visualizer::SceneObject, 3>;

    static visualizer::SceneObject makeBatch(visualizer::Topology topology,
                                             const std::array<float, 4>& color) {
        visualizer::SceneObject batch;
        batch.geometry.topology = topology;
        batch.material.color = color;
        batch.material.lit = topology == visualizer::Topology::TRIANGLES;
        return batch;
    }

    static visualizer::Vertex
    toRenderVertex(const geometry::point_t<geometry::defaultDim>& point,
                   const std::array<float, geometry::defaultDim>& normal = {}) {
        return {{static_cast<float>(point.x()),
                 static_cast<float>(point.y()),
                 static_cast<float>(point.z())},
                normal};
    }

    static void
    appendPrimitive(Batches& batches,
                    const geometry::polygon_t<geometry::defaultDim>& polygon) {
        auto& [pointBatch, segmentBatch, triangleBatch] = batches;
        const auto& points = polygon.points();
        if (points.size() != 3) {
            throw std::invalid_argument(
                "RenderDataPreparer: expected exactly 3 vertices");
        }

        if (points[0].distance(points[1]) == 0 &&
            points[0].distance(points[2]) == 0) {
            pointBatch.geometry.vertices.push_back(toRenderVertex(points[0]));
            return;
        }

        const auto areaVector = geometry::vector_t(points[0], points[1])
                                    .cross(geometry::vector_t(points[0], points[2]));
        if (areaVector.length() == 0) {
            const auto segment = polygon.maxEdge().value();
            auto& vertices = segmentBatch.geometry.vertices;
            vertices.push_back(toRenderVertex(segment.a()));
            vertices.push_back(toRenderVertex(segment.b()));
            return;
        }

        const auto n = polygon.unitNormal();
        const std::array<float, geometry::defaultDim> normal{
            static_cast<float>(n.x()),
            static_cast<float>(n.y()),
            static_cast<float>(n.z())};

        for (const auto& point : points) {
            triangleBatch.geometry.vertices.push_back(toRenderVertex(point, normal));
        }
    }
};

} // namespace triangles
