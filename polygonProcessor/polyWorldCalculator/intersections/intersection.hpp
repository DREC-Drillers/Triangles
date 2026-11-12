#pragma once

import geometry;

namespace triangles {

enum class RealTriangleForm { POINT, SEGMENT, TRIANGLE };

bool doIntersect(const geometry::triangle_t<3>& first,
                 const geometry::triangle_t<3>& second);

RealTriangleForm determineRealTriangleForm(const geometry::triangle_t<3>& triangle);

bool doSegmentIntersectTrianglePlain(const geometry::segment_t& segment,
                                     const geometry::triangle_t<3>& triangle);

geometry::point_t<3> getSegAndTrianglePlaneIntersectPoint(
    const geometry::segment_t& segment, const geometry::triangle_t<3>& triangle);

geometry::segment_t degenerateTriangleToSegment(const geometry::triangle_t<3>& segment);

bool doPointAndPointIntersect(const geometry::point_t<3>& firstPoint,
                              const geometry::point_t<3>& secondPoint);

bool doPointAndSegmentIntersect(const geometry::point_t<3>& point,
                                const geometry::segment_t& segment);

bool doPointAndTriangleIntersect(const geometry::point_t<3>& point,
                                 const geometry::triangle_t<3>& triangle);

bool doSegmentAndSegmentIntersect(const geometry::segment_t& firstSegment,
                                  const geometry::segment_t& secondSegment);

bool doSegmentAndTriangleIntersect(const geometry::segment_t& segment,
                                   const geometry::triangle_t<3>& triangle);

} // namespace triangles
