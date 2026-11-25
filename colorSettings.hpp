#pragma once

#include <array>

namespace triangles {

struct RenderStyle {
    std::array<float, 4> polyColor{0.7f, 0.7f, 0.7f, 1.0f};
    std::array<float, 4> intersectingPolyColor{1.0f, 0.0f, 0.0f, 1.0f};
};

namespace colorSettings {
inline constexpr bool kLightPaletteColors = false;
}

} // namespace triangles
