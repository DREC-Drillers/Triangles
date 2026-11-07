#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "aabb.hpp"
#include "tree/binary_tree.hpp"

namespace triangles {

class Bvh {
public:
    explicit Bvh(std::vector<Aabb> bounds);

    std::size_t size() const noexcept;
    bool empty() const noexcept;
    const Aabb& objectBounds(std::size_t object) const;

    template <typename Visitor>
    void query(const Aabb& bounds, Visitor&& visitor) const {
        if (const auto root = tree_.root()) {
            auto&& callback = std::forward<Visitor>(visitor);
            visitNode(*root, bounds, callback);
        }
    }

private:
    struct NodeData {
        Aabb bounds;
        std::size_t begin;
        std::size_t count;
    };

    using Tree = general_functions::BinaryTree<NodeData>;
    using NodeId = Tree::NodeId;

    NodeData makeNode(std::size_t begin, std::size_t count) const;
    std::size_t selectSplitAxis(std::size_t begin, std::size_t count,
                                const std::vector<Aabb::Coordinates>& centers) const;
    std::size_t partitionObjects(std::size_t begin, std::size_t count,
                                 std::size_t splitAxis,
                                 const std::vector<Aabb::Coordinates>& centers);
    void build(NodeId node, const std::vector<Aabb::Coordinates>& centers);

    template <typename Visitor>
    bool visitNode(NodeId id, const Aabb& bounds, Visitor& visitor) const {
        const auto& node = tree_.value(id);
        if (!node.bounds.overlaps(bounds)) {
            return true;
        }

        if (node.count != 0) {
            for (std::size_t offset = 0; offset < node.count; ++offset) {
                const auto object = objectIndices_.at(node.begin + offset);
                if (bounds_.at(object).overlaps(bounds) && !visitor(object)) {
                    return false;
                }
            }
            return true;
        }

        return visitNode(tree_.left(id).value(), bounds, visitor) &&
               visitNode(tree_.right(id).value(), bounds, visitor);
    }

    static constexpr std::size_t MAX_LEAF_SIZE = 4;

    std::vector<Aabb> bounds_;
    std::vector<std::size_t> objectIndices_;
    Tree tree_;
};

} // namespace triangles
