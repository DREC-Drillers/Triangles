#pragma once

#include <array>
#include <cstdint>

namespace triangles {

namespace settings {

inline constexpr std::int64_t kVertexCountUpperBound = 1'000'000;

inline constexpr std::array<float, 4> kGrayRgba{0.7f, 0.7f, 0.7f, 1.0f};
inline constexpr std::array<float, 4> kRedRgba{1.0f, 0.0f, 0.0f, 1.0f};

} // namespace settings

struct RenderStyle {
    std::array<float, 4> polyColor = settings::kGrayRgba;
    std::array<float, 4> intersectingPolyColor = settings::kRedRgba;
};

} // namespace triangles
