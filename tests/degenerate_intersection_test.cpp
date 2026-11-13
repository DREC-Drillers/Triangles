#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "polygonProcessor/polyWorldCalculator/intersections/intersection.hpp"
#include "polygonProcessor/polyWorldCalculator/polyWorldCalculator.hpp"

namespace {

using Coordinates = std::array<double, 3>;
using Vertices = std::array<Coordinates, 3>;

constexpr Vertices point(Coordinates p) {
    return {p, p, p};
}
constexpr Vertices segment(Coordinates a, Coordinates b) {
    return {a, b, a};
}
constexpr Vertices TRIANGLE{{{0, 0, 0}, {4, 0, 0}, {0, 4, 0}}};

struct PairCase {
    const char* name;
    Vertices first;
    Vertices second;
    bool expected;
};

std::ostream& operator<<(std::ostream& out, const PairCase& pair) {
    return out << pair.name;
}

class DegenerateIntersection : public testing::TestWithParam<PairCase> {};

void expectWorldIntersection(const geometry::triangle_t<3>& first,
                             const geometry::triangle_t<3>& second, bool expected) {
    triangles::InitPolysData input;
    for (const auto* triangle : {&first, &second}) {
        triangles::InitPolysData::PolygonPoints vertices;
        for (const auto& p : triangle->points()) {
            vertices.emplace_back(p.x(), p.y(), p.z());
        }
        input.data.push_back(std::move(vertices));
    }
    const triangles::PolyWorld world(input);
    const auto result =
        triangles::PolyWorldCalculator{}.calculateIntersections(world);
    const std::vector<const geometry::polygon_t<3>*> both{world.polys()[0].get(),
                                                          world.polys()[1].get()};
    const std::vector<const geometry::polygon_t<3>*> empty;
    EXPECT_EQ(&result.world, &world);
    EXPECT_EQ(result.intersectingPolys, expected ? both : empty);
    EXPECT_EQ(result.nonIntersectingPolys, expected ? empty : both);
}

TEST_P(DegenerateIntersection, HandlesAllVertexAndArgumentOrders) {
    const auto& pair = GetParam();
    for (std::size_t axis = 0; axis < 3; ++axis) {
        const auto makeTriangle = [axis](const Vertices& vertices,
                                         const auto& order) {
            const auto makePoint = [axis](const Coordinates& p) {
                return geometry::point_t<3>(
                    {p[axis], p[(axis + 1) % 3], p[(axis + 2) % 3]});
            };
            return geometry::triangle_t<3>(makePoint(vertices[order[0]]),
                                           makePoint(vertices[order[1]]),
                                           makePoint(vertices[order[2]]));
        };
        std::array<std::size_t, 3> firstOrder{0, 1, 2};
        do {
            std::array<std::size_t, 3> secondOrder{0, 1, 2};
            do {
                SCOPED_TRACE(testing::Message()
                             << "axis=" << axis << " first=" << firstOrder[0]
                             << firstOrder[1] << firstOrder[2] << " second="
                             << secondOrder[0] << secondOrder[1] << secondOrder[2]);
                const auto first = makeTriangle(pair.first, firstOrder);
                const auto second = makeTriangle(pair.second, secondOrder);
                EXPECT_EQ(triangles::doIntersect(first, second), pair.expected);
                EXPECT_EQ(triangles::doIntersect(second, first), pair.expected);
                expectWorldIntersection(first, second, pair.expected);
                expectWorldIntersection(second, first, pair.expected);
            } while (std::next_permutation(secondOrder.begin(), secondOrder.end()));
        } while (std::next_permutation(firstOrder.begin(), firstOrder.end()));
    }
}

INSTANTIATE_TEST_SUITE_P(
    Degeneracies, DegenerateIntersection,
    testing::Values(
        PairCase{"IdenticalPoints", point({1, 2, 3}), point({1, 2, 3}), true},
        PairCase{"DistinctPoints", point({1, 2, 3}), point({1, 2, 4}), false},
        PairCase{"PointInsideSegment",
                 point({2, 0, 0}),
                 segment({0, 0, 0}, {4, 0, 0}),
                 true},
        PairCase{"PointBeyondSegment",
                 point({5, 0, 0}),
                 segment({0, 0, 0}, {4, 0, 0}),
                 false},
        PairCase{"PointInsideTriangle", point({1, 1, 0}), TRIANGLE, true},
        PairCase{"PointOnTriangleEdge", point({2, 2, 0}), TRIANGLE, true},
        PairCase{"PointAtTriangleVertex", point({4, 0, 0}), TRIANGLE, true},
        PairCase{
            "PointOutsideTriangleWithinBounds", point({3, 3, 0}), TRIANGLE, false},
        PairCase{"PointAboveTriangle", point({1, 1, 1}), TRIANGLE, false},
        PairCase{"CollinearSegmentsOverlap",
                 segment({0, 0, 0}, {4, 0, 0}),
                 segment({2, 0, 0}, {6, 0, 0}),
                 true},
        PairCase{"CollinearSegmentsContained",
                 segment({0, 0, 0}, {4, 0, 0}),
                 segment({1, 0, 0}, {3, 0, 0}),
                 true},
        PairCase{"CollinearSegmentsTouch",
                 segment({0, 0, 0}, {4, 0, 0}),
                 segment({4, 0, 0}, {6, 0, 0}),
                 true},
        PairCase{"CollinearSegmentsDisjoint",
                 segment({0, 0, 0}, {4, 0, 0}),
                 segment({5, 0, 0}, {6, 0, 0}),
                 false},
        PairCase{"CrossingSegments",
                 segment({0, 0, 0}, {4, 0, 0}),
                 segment({2, -1, 0}, {2, 1, 0}),
                 true},
        PairCase{"SkewSegmentsWithOverlappingBounds",
                 segment({0, 0, 0}, {4, 4, 4}),
                 segment({0, 4, 3}, {4, 0, 3}),
                 false},
        PairCase{"SegmentPiercesTriangle",
                 segment({1, 1, -1}, {1, 1, 1}),
                 TRIANGLE,
                 true},
        PairCase{"SegmentPiercesPlaneOutsideTriangle",
                 segment({3, 3, -1}, {3, 3, 1}),
                 TRIANGLE,
                 false},
        PairCase{"CoplanarSegmentCrossesTriangle",
                 segment({-1, 1, 0}, {5, 1, 0}),
                 TRIANGLE,
                 true},
        PairCase{"CoplanarSegmentOutsideWithinBounds",
                 segment({3, 3, 0}, {4, 4, 0}),
                 TRIANGLE,
                 false},
        PairCase{"SegmentOverlapsTriangleEdge",
                 segment({-1, 0, 0}, {5, 0, 0}),
                 TRIANGLE,
                 true},
        PairCase{"SegmentTouchesTriangleVertex",
                 segment({-1, 1, 0}, {1, -1, 0}),
                 TRIANGLE,
                 true},
        PairCase{"SegmentEndsInsideTriangle",
                 segment({1, 1, 0}, {1, 1, 2}),
                 TRIANGLE,
                 true},
        PairCase{"TinySegmentAtPoint",
                 segment({0, 0, 0}, {1e-10, 0, 0}),
                 point({0, 0, 0}),
                 true},
        PairCase{"TinySegmentAwayFromPoint",
                 segment({0, 0, 0}, {1e-10, 0, 0}),
                 point({1, 0, 0}),
                 false},
        PairCase{"TinyCollinearSegment",
                 {{{0, 0, 0}, {1e-10, 0, 0}, {2e-10, 0, 0}}},
                 TRIANGLE,
                 true},
        PairCase{"TinySegmentsTouch",
                 segment({0, 0, 0}, {1e-10, 0, 0}),
                 segment({0, 0, 0}, {0, 1e-10, 0}),
                 true},
        PairCase{"TinySegmentsApart",
                 segment({0, 0, 0}, {1e-10, 0, 0}),
                 segment({1, 0, 0}, {1, 1e-10, 0}),
                 false},
        PairCase{"TinySegmentInsideTriangle",
                 segment({1, 1, 0}, {1 + 1e-10, 1, 0}),
                 TRIANGLE,
                 true},
        PairCase{"TinySegmentAboveTriangle",
                 segment({1, 1, 1}, {1 + 1e-10, 1, 1}),
                 TRIANGLE,
                 false},
        PairCase{"RepeatedVertexAtSegmentEnd",
                 {{{0, 0, 0}, {4, 0, 0}, {4, 0, 0}}},
                 point({2, 0, 0}),
                 true},
        PairCase{"MiddleVertexNotEndpoint",
                 {{{2, 0, 0}, {0, 0, 0}, {4, 0, 0}}},
                 segment({3, -1, 0}, {3, 1, 0}),
                 true},
        PairCase{"ObliqueCollinearEndpoint",
                 {{{-3, -6, -9}, {0, 0, 0}, {3, 6, 9}}},
                 point({3, 6, 9}),
                 true},
        PairCase{"ObliqueCollinearExtension",
                 {{{-3, -6, -9}, {0, 0, 0}, {3, 6, 9}}},
                 point({4, 8, 12}),
                 false},
        PairCase{"SegmentParallelToShortEdge",
                 segment({-1, 2, 0}, {1, 2, 0}),
                 {{{0, 0, 0}, {1e-10, 0, 0}, {0, 4, 0}}},
                 true},
        PairCase{"PointOutsideAlmostCollinearTriangle",
                 point({1, 1, 0}),
                 {{{0, 0, 0}, {1, 0, 0}, {2, 1e-10, 0}}},
                 false},
        PairCase{"TinyTrianglesApart",
                 {{{0, 0, 0}, {1e-5, 0, 0}, {0, 1e-5, 0}}},
                 {{{1, 1, 0}, {1 + 1e-5, 1, 0}, {1, 1 + 1e-5, 0}}},
                 false},
        PairCase{"TinyTrianglesOnParallelPlanes",
                 {{{0, 0, 0}, {1e-5, 0, 0}, {0, 1e-5, 0}}},
                 {{{0, 0, 1}, {1e-5, 0, 1}, {0, 1e-5, 1}}},
                 false},
        PairCase{"ThinTrianglesApart",
                 {{{0, 0, 0}, {4, 0, 0}, {2, 1e-10, 0}}},
                 {{{1, 1, 0}, {5, 1, 0}, {3, 1 + 1e-10, 0}}},
                 false},
        PairCase{"TinyTrianglesOverlap",
                 {{{0, 0, 0}, {1e-5, 0, 0}, {0, 1e-5, 0}}},
                 {{{2e-6, 2e-6, 0}, {4e-6, 2e-6, 0}, {2e-6, 4e-6, 0}}},
                 true},
        PairCase{"SegmentNearPlaneOutsideTriangle",
                 segment({5, 5, 0.5e-9}, {6, 6, 1}),
                 TRIANGLE,
                 false},
        PairCase{"OffsetParallelSegments",
                 segment({0, 0, 0}, {4, 4, 4}),
                 segment({0, 1, 0}, {4, 5, 4}),
                 false}),
    [](const testing::TestParamInfo<PairCase>& info) {
        return std::string(info.param.name);
    });

// Independent separating-axis oracle. Small integer coordinates keep every
// determinant and projection exact; production predicates use floating point.
using IntegerPoint = std::array<std::int64_t, 3>;
using IntegerTriangle = std::array<IntegerPoint, 3>;

IntegerPoint subtract(const IntegerPoint& a, const IntegerPoint& b) {
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

IntegerPoint cross(const IntegerPoint& a, const IntegerPoint& b) {
    return {a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]};
}

std::int64_t dot(const IntegerPoint& a, const IntegerPoint& b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

bool exactIntersection(const IntegerTriangle& first, const IntegerTriangle& second) {
    const auto separated = [&](const IntegerPoint& axis) {
        std::array<std::int64_t, 3> a{}, b{};
        for (std::size_t i = 0; i < 3; ++i) {
            a[i] = dot(first[i], axis);
            b[i] = dot(second[i], axis);
        }
        return *std::max_element(a.begin(), a.end()) <
                   *std::min_element(b.begin(), b.end()) ||
               *std::max_element(b.begin(), b.end()) <
                   *std::min_element(a.begin(), a.end());
    };
    const std::array edges{subtract(first[1], first[0]),
                           subtract(first[2], first[1]),
                           subtract(first[0], first[2]),
                           subtract(second[1], second[0]),
                           subtract(second[2], second[1]),
                           subtract(second[0], second[2])};
    const auto offset = subtract(second[0], first[0]);
    if (separated(offset))
        return false;
    for (const auto& edge : edges) {
        // The extra axis handles points and parallel, non-collinear segments.
        if (separated(edge) || separated(cross(edge, cross(edge, offset))))
            return false;
    }
    for (std::size_t i = 0; i < edges.size(); ++i) {
        for (std::size_t j = i + 1; j < edges.size(); ++j) {
            const auto normal = cross(edges[i], edges[j]);
            if (separated(normal))
                return false;
            for (const auto& edge : edges) {
                // In-plane axes are needed for coplanar triangles and segments.
                if (separated(cross(normal, edge)))
                    return false;
            }
        }
    }
    return true;
}

geometry::triangle_t<3> makeTriangle(const IntegerTriangle& triangle,
                                     double scale = 1) {
    const auto p = [scale](const IntegerPoint& vertex) {
        return geometry::point_t<3>(
            {scale * vertex[0], scale * vertex[1], scale * vertex[2]});
    };
    return geometry::triangle_t<3>(p(triangle[0]), p(triangle[1]), p(triangle[2]));
}

TEST(IntersectionOracle, ExhaustsTrianglesOnThreeByThreeGrid) {
    std::vector<IntegerPoint> points;
    for (std::int64_t x = -1; x <= 1; ++x) {
        for (std::int64_t y = -1; y <= 1; ++y)
            points.push_back({x, y, 0});
    }
    std::vector<IntegerTriangle> triangles;
    for (std::size_t a = 0; a < points.size(); ++a) {
        for (std::size_t b = a; b < points.size(); ++b) {
            for (std::size_t c = b; c < points.size(); ++c) {
                triangles.push_back({points[a], points[b], points[c]});
            }
        }
    }
    ASSERT_EQ(triangles.size(), 165U);
    for (std::size_t first = 0; first < triangles.size(); ++first) {
        for (std::size_t second = first; second < triangles.size(); ++second) {
            SCOPED_TRACE(testing::Message()
                         << "first=" << first << " second=" << second);
            const bool expected =
                exactIntersection(triangles[first], triangles[second]);
            const auto a = makeTriangle(triangles[first]);
            const auto b = makeTriangle(triangles[second]);
            ASSERT_EQ(triangles::doIntersect(a, b), expected);
            ASSERT_EQ(triangles::doIntersect(b, a), expected);
            expectWorldIntersection(a, b, expected);
            expectWorldIntersection(b, a, expected);
        }
    }
}

TEST(IntersectionOracle, MatchesExactSpatialGeometryAtDifferentScales) {
    std::mt19937 random(20261112);
    std::uniform_int_distribution<int> coordinate(-4, 4);
    const auto makeVertices = [&](int form) {
        const auto p = [&]() -> IntegerPoint {
            return {coordinate(random), coordinate(random), coordinate(random)};
        };
        const auto a = p();
        const auto b = form == 0 ? a : p();
        const auto c =
            form == 2
                ? p()
                : IntegerPoint{2 * b[0] - a[0], 2 * b[1] - a[1], 2 * b[2] - a[2]};
        return IntegerTriangle{a, b, c};
    };
    for (int i = 0; i < 5000; ++i) {
        const auto first = makeVertices(i % 3);
        const auto second = makeVertices((i / 3) % 3);
        const bool expected = exactIntersection(first, second);
        // Binary scales preserve exact collinearity of integer input vertices.
        for (double scale : {1.0 / 65536, 1.0, 1024.0}) {
            SCOPED_TRACE(testing::Message() << "sample=" << i << " scale=" << scale);
            const auto a = makeTriangle(first, scale);
            const auto b = makeTriangle(second, scale);
            ASSERT_EQ(triangles::doIntersect(a, b), expected);
            ASSERT_EQ(triangles::doIntersect(b, a), expected);
            expectWorldIntersection(a, b, expected);
            expectWorldIntersection(b, a, expected);
        }
    }
}

TEST(DegenerateCalculator, PartitionsMixedSpatialGroupsAgainstExactOracle) {
    std::mt19937 random(20261113);
    std::uniform_int_distribution<int> coordinate(-4, 4);
    std::vector<IntegerTriangle> vertices;
    for (int group = 0; group < 20; ++group) {
        for (int object = 0; object < 8; ++object) {
            const auto p = [&]() -> IntegerPoint {
                return {group * 64 + coordinate(random),
                        coordinate(random),
                        coordinate(random)};
            };
            const auto a = p();
            const auto b = object % 3 == 0 ? a : p();
            const auto c = object % 3 == 2 ? p()
                                           : IntegerPoint{2 * b[0] - a[0],
                                                          2 * b[1] - a[1],
                                                          2 * b[2] - a[2]};
            vertices.push_back({a, b, c});
        }
        // Guaranteed contacts and isolated points ensure both result lists
        // remain populated regardless of the random geometry.
        const IntegerPoint isolated{group * 64, 20, 20};
        const IntegerPoint shared{group * 64, 30, 30};
        vertices.push_back({isolated, isolated, isolated});
        vertices.push_back({shared, shared, shared});
        vertices.push_back({shared, shared, shared});
    }
    std::shuffle(vertices.begin(), vertices.end(), random);
    std::vector<bool> marked(vertices.size(), false);
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        for (std::size_t j = 0; j < i; ++j) {
            if (exactIntersection(vertices[i], vertices[j])) {
                marked[i] = marked[j] = true;
            }
        }
    }
    ASSERT_NE(std::count(marked.begin(), marked.end(), true), 0);
    ASSERT_NE(std::count(marked.begin(), marked.end(), false), 0);
    for (double scale : {1.0 / 65536, 1.0, 1024.0}) {
        SCOPED_TRACE(testing::Message() << "scale=" << scale);
        triangles::InitPolysData input;
        for (const auto& triangle : vertices) {
            triangles::InitPolysData::PolygonPoints polygon;
            for (const auto& p : triangle) {
                polygon.emplace_back(scale * p[0], scale * p[1], scale * p[2]);
            }
            input.data.push_back(std::move(polygon));
        }
        const triangles::PolyWorld world(input);
        std::vector<const geometry::polygon_t<3>*> intersecting, nonIntersecting;
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            (marked[i] ? intersecting : nonIntersecting)
                .push_back(world.polys()[i].get());
        }
        const auto result =
            triangles::PolyWorldCalculator{}.calculateIntersections(world);
        EXPECT_EQ(result.intersectingPolys, intersecting);
        EXPECT_EQ(result.nonIntersectingPolys, nonIntersecting);
    }
}

TEST(DegenerateCalculator, DoesNotIntersectSinglePointOrSegmentWithItself) {
    for (const auto& vertices : {point({1, 2, 3}), segment({0, 0, 0}, {4, 0, 0})}) {
        triangles::InitPolysData input;
        triangles::InitPolysData::PolygonPoints polygon;
        for (const auto& p : vertices)
            polygon.emplace_back(p[0], p[1], p[2]);
        input.data.push_back(std::move(polygon));
        const triangles::PolyWorld world(input);
        const auto result =
            triangles::PolyWorldCalculator{}.calculateIntersections(world);
        EXPECT_TRUE(result.intersectingPolys.empty());
        ASSERT_EQ(result.nonIntersectingPolys.size(), 1U);
        EXPECT_EQ(result.nonIntersectingPolys[0], world.polys()[0].get());
    }
}

TEST(DegenerateCalculator, ReportsDenseCoincidentDegeneraciesOnceInInputOrder) {
    for (const auto& vertices : {point({1, 2, 3}), segment({0, 0, 0}, {4, 0, 0})}) {
        triangles::InitPolysData input;
        triangles::InitPolysData::PolygonPoints polygon;
        for (const auto& p : vertices)
            polygon.emplace_back(p[0], p[1], p[2]);
        input.data.assign(64, polygon);
        const triangles::PolyWorld world(input);
        const auto result =
            triangles::PolyWorldCalculator{}.calculateIntersections(world);
        EXPECT_TRUE(result.nonIntersectingPolys.empty());
        ASSERT_EQ(result.intersectingPolys.size(), world.polys().size());
        for (std::size_t i = 0; i < world.polys().size(); ++i) {
            EXPECT_EQ(result.intersectingPolys[i], world.polys()[i].get());
        }
    }
}

} // namespace
