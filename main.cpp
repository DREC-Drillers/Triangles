#include <exception>
#include <filesystem>
#include <iostream>

#include "parser/parser.hpp"
#include "polygonProcessor/polyWorld.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: triangles_input <scene-file>\n";
        return 1;
    }

    try {
        const auto data = triangles::Parser{}.parse(std::filesystem::path{argv[1]});
        const triangles::PolyWorld<3> world(data);

        std::cout << "Read " << world.polys().size() << " polygons\n";
        std::cout << "PolyWorld created\n";

        for (std::size_t i = 0; i < world.polys().size(); ++i) {
            const auto& polygon = world.polys()[i];
            const auto normal = polygon.unitNormal();
            std::cout << "Polygon " << i + 1 << ":\n";
            std::cout << "  Vertices: " << polygon.numOfPoints() << '\n';
            std::cout << "  Area: " << polygon.area() << '\n';
            std::cout << "  Unit normal: " << normal.x() << ' ' << normal.y() << ' ' << normal.z() << '\n';
            std::cout << "  Points:\n";
            for (const auto& point : polygon.points()) {
                std::cout << "    " << point.x() << ' ' << point.y() << ' '
                          << point.z() << '\n';
            }
        }

    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
