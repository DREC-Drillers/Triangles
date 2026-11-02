#include <exception>
#include <filesystem>
#include <iostream>

#include "parser/parser.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: triangles_input <scene-file>\n";
        return 1;
    }

    try {
        const auto poly_arr = triangles::Parser{}.parse(std::filesystem::path{argv[1]});
        std::cout << "Read " << poly_arr.data.size() << " triangles\n";
        for (std::size_t i = 0; i < poly_arr.data.size(); ++i) {
            std::cout << "Triangle " << i + 1 << ":\n";
            for (const auto& point : poly_arr.data[i]) {
                std::cout << "  " << point.x() << ' ' << point.y() << ' '
                          << point.z() << '\n';
            }
        }

    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
