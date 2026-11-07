#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

#include "cameraController/orbitCameraController.hpp"
#if __has_include("core/backendFactory.hpp")
#include "core/backendFactory.hpp"
#include "core/visualizer.hpp"
#else
#include "backendFactory.hpp"
#include "visualizer.hpp"
#endif
#include "parser/parser.hpp"
#include "polygonProcessor/polyWorldCalculator/polyWorldCalculator.hpp"
#include "polygonProcessor/renderDataPreparer.hpp"

int main(int argc, char* argv[]) {
    const std::span<char* const> arguments(argv, static_cast<std::size_t>(argc));
    const bool noVisualization =
        argc == 3 && std::string_view(arguments[2]) == "--no-visualization";
    if (argc != 2 && !noVisualization) {
        std::cerr << "Usage: triangles <input-file> [--no-visualization]\n";
        return 1;
    }

    try {
        using Clock = std::chrono::steady_clock;

        const auto start = Clock::now();
        const auto input =
            triangles::Parser{}.parse(std::filesystem::path{arguments[1]});
        const auto parsed = Clock::now();
        const triangles::PolyWorld<3> world(input);
        const auto worldCreated = Clock::now();
        const auto result =
            triangles::PolyWorldCalculator{}.calculateIntersections(world);
        const auto calculated = Clock::now();

        visualizer::Scene scene;
        for (auto& object : triangles::RenderDataPreparer{}.prepare(result)) {
            object.material.lit = false;
            scene.addObject(std::move(object));
        }
        const auto scenePrepared = Clock::now();

        const auto milliseconds = [](auto begin, auto end) {
            return std::chrono::duration<double, std::milli>(end - begin).count();
        };

        std::cout << "Triangles: " << world.polys().size()
                  << "\nIntersecting: " << result.intersectingPolys.size()
                  << "\nNon-intersecting: " << result.nonIntersectingPolys.size()
                  << "\nParsing_ms: " << milliseconds(start, parsed)
                  << "\nWorld_ms: " << milliseconds(parsed, worldCreated)
                  << "\nIntersections_ms: " << milliseconds(worldCreated, calculated)
                  << "\nScene_ms: " << milliseconds(calculated, scenePrepared)
                  << "\nTotal_ms: " << milliseconds(start, scenePrepared)
                  << std::endl;

        if (noVisualization) {
            return 0;
        }

        visualizer::Camera camera;
        constexpr float CAMERA_DISTANCE = 40;
        camera.setPosition({0, 0, CAMERA_DISTANCE});
        const auto cameraId = scene.addCamera(std::move(camera));

        visualizer::Viewer viewer(
            visualizer::BackendFactory::create(visualizer::BackendSettings{}));
        viewer.addView(scene,
                       cameraId,
                       {},
                       std::make_unique<visualizer::OrbitCameraController>());
        viewer.run();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
