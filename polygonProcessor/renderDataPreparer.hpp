#pragma once

#include <array>
#include <cstddef>
#include <stdexcept>
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
        objects.reserve(intersections.nonIntersectingPolys.size() +
                        intersections.intersectingPolys.size());

        const auto append =
            [&](const std::vector<const geometry::polygon_t<geometry::defaultDim>*>&
                    polygons,
                const std::array<float, 4>& color) {
                for (const auto* polygon : polygons) {
                    if (polygon == nullptr) {
                        throw std::invalid_argument(
                            "RenderDataPreparer: null polygon");
                    }
                    objects.push_back(prepareTriangle(*polygon, color));
                }
            };

        append(intersections.nonIntersectingPolys, style.polyColor);
        append(intersections.intersectingPolys, style.intersectingPolyColor);
        return objects;
    }

private:
    static visualizer::Vertex
    toRenderVertex(const geometry::point_t<geometry::defaultDim>& point,
                   const std::array<float, geometry::defaultDim>& normal = {}) {
        return {{static_cast<float>(point.x()),
                 static_cast<float>(point.y()),
                 static_cast<float>(point.z())},
                normal};
    }

    static visualizer::SceneObject
    preparePoint(const geometry::point_t<geometry::defaultDim>& point,
                 const std::array<float, 4>& color) {
        visualizer::SceneObject object;
        object.geometry.topology = visualizer::Topology::POINTS;
        object.material.color = color;
        object.material.lit = false;
        object.geometry.vertices.push_back(toRenderVertex(point));
        return object;
    }

    static visualizer::SceneObject
    prepareSegment(const geometry::polygon_t<geometry::defaultDim>& polygon,
                   const std::array<float, 4>& color) {
        const auto segment = polygon.maxEdge().value();

        visualizer::SceneObject object;
        object.geometry.topology = visualizer::Topology::LINES;
        object.material.color = color;
        object.material.lit = false;
        object.geometry.vertices = {toRenderVertex(segment.a()),
                                    toRenderVertex(segment.b())};
        return object;
    }

    static visualizer::SceneObject
    prepareTriangle(const geometry::polygon_t<geometry::defaultDim>& polygon,
                    const std::array<float, 4>& color) {
        const auto& points = polygon.points();
        if (points.size() != 3) {
            throw std::invalid_argument(
                "RenderDataPreparer: expected exactly 3 vertices");
        }

        if (points[0].distance(points[1]) == 0 &&
            points[0].distance(points[2]) == 0) {
            return preparePoint(points[0], color);
        }

        const auto areaVector = geometry::vector_t(points[0], points[1])
                                    .cross(geometry::vector_t(points[0], points[2]));
        if (areaVector.length() == 0) {
            return prepareSegment(polygon, color);
        }

        visualizer::SceneObject object;
        object.geometry.topology = visualizer::Topology::TRIANGLES;
        object.material.color = color;
        object.material.lit = true;
        object.geometry.vertices.reserve(3);

        const auto n = polygon.unitNormal();
        const std::array<float, geometry::defaultDim> normal{
            static_cast<float>(n.x()),
            static_cast<float>(n.y()),
            static_cast<float>(n.z())};

        for (const auto& point : points) {
            object.geometry.vertices.push_back(toRenderVertex(point, normal));
        }
        return object;
    }
};

} // namespace triangles
