#pragma once

#include <cstddef>
#include <vector>

#include "initPolysData.hpp"
#include "intersectionData.hpp"
#include "polyCalculator.hpp"
#include "polygons.hpp"

namespace triangles {

template <std::size_t DimWorld, std::size_t DimPoly>
class PolyWorld {
private:
    Polygons polys_;
    IntersectionData intersectionData_;

public:
    explicit PolyWorld(const InitPolysData& initData) {
        polys_.reserve(initData.data.size());
        for (const auto& polygon : initData.data) {
            std::vector<geometry::point_t> points;
            points.reserve(polygon.size());
            for (const auto& point : polygon) {
                points.emplace_back(point.x(), point.y(), point.z());
            }
            polys_.push_back(makePolygon(std::move(points)));
        }
    }

    const IntersectionData& calculateIntersections() {
        PolyCalculator calculator;
        intersectionData_ = calculator.calculateIntersections(polys_);
        return intersectionData_;
    }

    const Polygons& polys() const { return polys_; }

    const IntersectionData& intersectionData() const { return intersectionData_; }
};

} // namespace triangles
