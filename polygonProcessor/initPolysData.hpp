#pragma once

#include <cmath>
#include <vector>

namespace triangles {

struct InitPolysData {
    class Point {
    public:
        Point() = default;
        Point(double x, double y, double z) : x_(x), y_(y), z_(z) {}

        double x() const { return x_; }
        double y() const { return y_; }
        double z() const { return z_; }
        double distance(const Point& other) const {
            return std::hypot(x_ - other.x_, y_ - other.y_, z_ - other.z_);
        }

    private:
        double x_ = 0.0;
        double y_ = 0.0;
        double z_ = 0.0;
    };

    using PolygonPoints = std::vector<Point>;

    std::vector<PolygonPoints> data;
};

} // namespace triangles
