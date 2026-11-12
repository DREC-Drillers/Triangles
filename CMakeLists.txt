cmake_minimum_required(VERSION 3.28)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not supported. Use a separate build directory.")
endif()

project(Triangles VERSION 0.1.0 LANGUAGES CXX)

option(TRIANGLES_BUILD_APP "Build the Triangles application" ON)

include(CTest)

function(triangles_configure_target target)
    target_compile_features(${target} PRIVATE cxx_std_17)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

if(TRIANGLES_BUILD_APP OR BUILD_TESTING)
    set(GENERAL_FUNCTIONS_SOURCE_DIR "" CACHE PATH
        "Optional local generalFunctions checkout; otherwise use the installed package")

    if(GENERAL_FUNCTIONS_SOURCE_DIR)
        if(NOT EXISTS "${GENERAL_FUNCTIONS_SOURCE_DIR}/CMakeLists.txt")
            message(FATAL_ERROR
                "GENERAL_FUNCTIONS_SOURCE_DIR must contain the generalFunctions CMake project")
        endif()

        set(GENERAL_FUNCTIONS_BUILD_LEXER ON)
        set(GENERAL_FUNCTIONS_BUILD_FILES ON)
        set(GENERAL_FUNCTIONS_BUILD_GEOMETRY ON)
        set(GENERAL_FUNCTIONS_BUILD_TESTS OFF)
        set(GENERAL_FUNCTIONS_BUILD_TREE ON)
        set(GENERAL_FUNCTIONS_BUILD_BVH ON)
        add_subdirectory("${GENERAL_FUNCTIONS_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}/general-functions" EXCLUDE_FROM_ALL)
    else()
        find_package(GeneralFunctions CONFIG REQUIRED COMPONENTS lexer files geometry bvh)
    endif()
endif()

if(TRIANGLES_BUILD_APP OR BUILD_TESTING)
    add_library(triangles_geometry STATIC
        polygonProcessor/polyWorldCalculator/intersections/intersection.cpp
        polygonProcessor/polyWorldCalculator/intersections/triangleBounds.cpp
        polygonProcessor/polyWorldCalculator/intersections/triangleIntersection.cpp
        polygonProcessor/polyWorldCalculator/polyWorldCalculator.cpp
    )
    target_include_directories(triangles_geometry PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(triangles_geometry PUBLIC
        GeneralFunctions::geometry GeneralFunctions::bvh)
    set_property(TARGET triangles_geometry PROPERTY CXX_SCAN_FOR_MODULES ON)
    triangles_configure_target(triangles_geometry)
endif()

if(TRIANGLES_BUILD_APP)
    add_executable(triangles
        main.cpp
        parser/parser.cpp
    )

    target_link_libraries(triangles PRIVATE
        triangles_geometry
        GeneralFunctions::lexer
        GeneralFunctions::files
        GeneralFunctions::geometry)

    set(VISUALIZER_SOURCE_DIR "" CACHE PATH "Optional local 3D-Visualizer checkout")
    if(VISUALIZER_SOURCE_DIR)
        set(VISUALIZER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(VISUALIZER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        add_subdirectory("${VISUALIZER_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}/visualizer" EXCLUDE_FROM_ALL)
    else()
        find_package(Visualizer 0.1 CONFIG REQUIRED)
    endif()
    target_link_libraries(triangles PRIVATE Visualizer::visualizer)

    set_property(TARGET triangles PROPERTY CXX_SCAN_FOR_MODULES ON)

    target_include_directories(triangles PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/polygonProcessor"
    )

    triangles_configure_target(triangles)
endif()

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    include(GoogleTest)

    # Installed packages expose libraries; the Nix input supplies their test sources.
    set(GENERAL_FUNCTIONS_TEST_SOURCE_DIR "$ENV{GENERAL_FUNCTIONS_TEST_SOURCE_DIR}"
        CACHE PATH "generalFunctions sources containing lexer and token stream tests")
    if(GENERAL_FUNCTIONS_SOURCE_DIR)
        set(general_functions_test_dir "${GENERAL_FUNCTIONS_SOURCE_DIR}")
    else()
        set(general_functions_test_dir "${GENERAL_FUNCTIONS_TEST_SOURCE_DIR}")
    endif()

    foreach(component lexer token_stream)
        set(test_source "${general_functions_test_dir}/tests/${component}_test.cpp")
        if(NOT EXISTS "${test_source}")
            message(FATAL_ERROR
                "generalFunctions test source not found: ${test_source}. Use nix develop "
                "or set GENERAL_FUNCTIONS_TEST_SOURCE_DIR to a checkout with GoogleTest tests.")
        endif()
        add_executable(${component}_tests "${test_source}")
        target_link_libraries(${component}_tests PRIVATE GeneralFunctions::lexer GTest::gtest_main)
        triangles_configure_target(${component}_tests)
        gtest_discover_tests(${component}_tests)
    endforeach()

    add_executable(parser_tests tests/parser_test.cpp parser/parser.cpp)
    target_include_directories(parser_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/polygonProcessor")
    target_link_libraries(parser_tests PRIVATE
        GeneralFunctions::lexer GeneralFunctions::files
        GeneralFunctions::geometry GTest::gtest_main)
    set_property(TARGET parser_tests PROPERTY CXX_SCAN_FOR_MODULES ON)
    target_compile_definitions(parser_tests PRIVATE
        TRIANGLES_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
    triangles_configure_target(parser_tests)
    gtest_discover_tests(parser_tests)

    foreach(component intersection triangle_intersection poly_world_calculator)
        add_executable(${component}_tests tests/${component}_test.cpp)
        target_include_directories(${component}_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
        target_link_libraries(${component}_tests PRIVATE
            triangles_geometry GeneralFunctions::geometry GTest::gtest_main)
        set_property(TARGET ${component}_tests PROPERTY CXX_SCAN_FOR_MODULES ON)
        triangles_configure_target(${component}_tests)
        gtest_discover_tests(${component}_tests)
    endforeach()
endif()
