#pragma once

#include <array>

namespace triangles {

class Aabb {
public:
    using Coordinates = std::array<double, 3>;

    Aabb(const Coordinates& min, const Coordinates& max);

    const Coordinates& min() const noexcept;
    const Coordinates& max() const noexcept;

    bool overlaps(const Aabb& other) const noexcept;
    void expand(const Aabb& other) noexcept;
    Coordinates center() const noexcept;

private:
    Coordinates min_;
    Coordinates max_;
};

} // namespace triangles
