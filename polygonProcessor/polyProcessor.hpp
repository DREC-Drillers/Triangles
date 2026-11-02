#pragma once

#include <cstddef>
#include <utility>

#include "../render/renderer.hpp"
#include "initPolysData.hpp"
#include "intersectionData.hpp"
#include "polyWorld.hpp"
#include "renderDataPreparer.hpp"

namespace triangles {

template <std::size_t dimWorld, std::size_t dimPoly>
class PolyProcessor {
private:
    PolyWorld<dimWorld, dimPoly> world_;
    Renderer renderer_;
    RenderStyle renderStyle_;

public:
    explicit PolyProcessor(PolyWorld<dimWorld, dimPoly> world)
        : world_(std::move(world)) {}

    const PolyWorld<dimWorld, dimPoly>& world() const { return world_; }

    const IntersectionData& calculateIntersections() {
        return world_.calculateIntersections();
    }

    void drawPolys() {
        RenderDataPreparer<dimWorld, dimPoly> preparer;
        RenderData data = preparer.prepare(
            world_.polys(), world_.intersectionData(), renderStyle_);
        renderer_.draw(data);
    }
};

} // namespace triangles
