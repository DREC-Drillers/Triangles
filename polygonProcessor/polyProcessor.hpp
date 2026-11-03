#pragma once

#include <cstddef>
#include <utility>

#include "../render/renderer.hpp"
#include "initPolysData.hpp"
#include "intersectionData.hpp"
#include "polyWorld.hpp"
#include "renderDataPreparer.hpp"

namespace triangles {

template <std::size_t dimWorld>
class PolyProcessor {
private:
    PolyWorld<dimWorld> world_;
    Renderer renderer_;
    RenderStyle renderStyle_;

public:
    explicit PolyProcessor(PolyWorld<dimWorld> world) : world_(std::move(world)) {}

    const PolyWorld<dimWorld>& world() const { return world_; }

    const IntersectionData& calculateIntersections() {
        return world_.calculateIntersections();
    }

    void drawPolys() {
        RenderDataPreparer<dimWorld> preparer;
        RenderData data = preparer.prepare(
            world_.polys(), world_.intersectionData(), renderStyle_);
        renderer_.draw(data);
    }
};

} // namespace triangles
