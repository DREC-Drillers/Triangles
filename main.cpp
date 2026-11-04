#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <span>
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
#include "polygonProcessor/polyCalculator.hpp"
#include "polygonProcessor/renderDataPreparer.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: triangles <input-file>\n";
        return 1;
    }

    try {
        const std::span<char* const> arguments(argv, static_cast<std::size_t>(argc));
        const auto input =
            triangles::Parser{}.parse(std::filesystem::path{arguments[1]});
        const triangles::PolyWorld<3> world(input);
        const auto result =
            triangles::PolyCalculator{}.calculateIntersections(world);
        std::cout << "Triangles: " << world.polys().size()
                  << "\nIntersecting: " << result.intersectingPolys.size()
                  << "\nNon-intersecting: " << result.nonIntersectingPolys.size()
                  << std::endl;

        visualizer::Scene scene;
        for (auto& object : triangles::RenderDataPreparer{}.prepare(result)) {
            object.material.lit = false;
            scene.addObject(std::move(object));
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
