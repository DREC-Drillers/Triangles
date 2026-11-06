#include "aabb.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace triangles {

Aabb::Aabb(const Coordinates& min, const Coordinates& max) : min_(min), max_(max) {
    for (std::size_t axis = 0; axis < min_.size(); ++axis) {
        if (!std::isfinite(min_.at(axis)) || !std::isfinite(max_.at(axis)) ||
            min_.at(axis) > max_.at(axis)) {
            throw std::invalid_argument(
                "Aabb: bounds must be finite and min <= max");
        }
    }
}

const Aabb::Coordinates& Aabb::min() const noexcept {
    return min_;
}

const Aabb::Coordinates& Aabb::max() const noexcept {
    return max_;
}

bool Aabb::overlaps(const Aabb& other) const noexcept {
    for (std::size_t axis = 0; axis < min_.size(); ++axis) {
        if (max_.at(axis) < other.min_.at(axis) ||
            other.max_.at(axis) < min_.at(axis)) {
            return false;
        }
    }

    return true;
}

void Aabb::expand(const Aabb& other) noexcept {
    for (std::size_t axis = 0; axis < min_.size(); ++axis) {
        min_.at(axis) = std::min(min_.at(axis), other.min_.at(axis));
        max_.at(axis) = std::max(max_.at(axis), other.max_.at(axis));
    }
}

Aabb::Coordinates Aabb::center() const noexcept {
    Coordinates result{};
    for (std::size_t axis = 0; axis < result.size(); ++axis) {
        const double lower = min_.at(axis);
        const double upper = max_.at(axis);
        result.at(axis) = lower <= 0 && upper >= 0 ? (lower + upper) / 2
                                                   : lower + (upper - lower) / 2;
    }

    return result;
}

} // namespace triangles
