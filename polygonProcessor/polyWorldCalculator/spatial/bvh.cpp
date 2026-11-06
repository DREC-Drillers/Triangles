#include "bvh.hpp"

#include <algorithm>
#include <numeric>
#include <utility>

namespace triangles {

Bvh::Bvh(std::vector<Aabb> bounds) : bounds_(std::move(bounds)) {
    if (bounds_.empty()) {
        return;
    }

    tree_.reserve(bounds_.size());
    objectIndices_.resize(bounds_.size());
    std::iota(objectIndices_.begin(), objectIndices_.end(), std::size_t{0});

    std::vector<Aabb::Coordinates> centers;
    centers.reserve(bounds_.size());
    for (const auto& box : bounds_) {
        centers.push_back(box.center());
    }

    const auto root = tree_.addRoot(makeNode(0, bounds_.size()));
    build(root, centers);
}

std::size_t Bvh::size() const noexcept {
    return bounds_.size();
}

bool Bvh::empty() const noexcept {
    return bounds_.empty();
}

const Aabb& Bvh::objectBounds(std::size_t object) const {
    return bounds_.at(object);
}

Bvh::NodeData Bvh::makeNode(std::size_t begin, std::size_t count) const {
    Aabb bounds = bounds_.at(objectIndices_.at(begin));
    for (std::size_t offset = 1; offset < count; ++offset) {
        bounds.expand(bounds_.at(objectIndices_.at(begin + offset)));
    }
    return {bounds, begin, count};
}

void Bvh::build(NodeId node, const std::vector<Aabb::Coordinates>& centers) {
    const auto begin = tree_.value(node).begin;
    const auto count = tree_.value(node).count;
    if (count <= MAX_LEAF_SIZE) {
        return;
    }

    auto minCenter = centers.at(objectIndices_.at(begin));
    auto maxCenter = minCenter;
    for (std::size_t offset = 1; offset < count; ++offset) {
        const auto& center = centers.at(objectIndices_.at(begin + offset));
        for (std::size_t axis = 0; axis < center.size(); ++axis) {
            minCenter.at(axis) = std::min(minCenter.at(axis), center.at(axis));
            maxCenter.at(axis) = std::max(maxCenter.at(axis), center.at(axis));
        }
    }

    std::size_t axis = 0;
    for (std::size_t candidate = 1; candidate < minCenter.size(); ++candidate) {
        const auto extent = [&](std::size_t dimension) {
            return maxCenter.at(dimension) / 2 - minCenter.at(dimension) / 2;
        };
        if (extent(candidate) > extent(axis)) {
            axis = candidate;
        }
    }

    const auto middle = begin + count / 2;
    using Difference = std::vector<std::size_t>::difference_type;
    std::nth_element(objectIndices_.begin() + static_cast<Difference>(begin),
                     objectIndices_.begin() + static_cast<Difference>(middle),
                     objectIndices_.begin() + static_cast<Difference>(begin + count),
                     [&](std::size_t first, std::size_t second) {
                         const auto a = centers.at(first).at(axis);
                         const auto b = centers.at(second).at(axis);
                         return a < b || (a == b && first < second);
                     });

    tree_.value(node).count = 0;
    const auto left = tree_.addLeft(node, makeNode(begin, middle - begin));
    build(left, centers);
    const auto right =
        tree_.addRight(node, makeNode(middle, begin + count - middle));
    build(right, centers);
}

} // namespace triangles
