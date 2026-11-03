cmake_minimum_required(VERSION 3.28)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not supported. Use a separate build directory.")
endif()

project(Triangles VERSION 0.1.0 LANGUAGES CXX)

option(TRIANGLES_BUILD_APP "Build the triangle input parser" OFF)

include(CTest)

function(triangles_configure_target target)
    target_compile_features(${target} PRIVATE cxx_std_20)
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
        add_subdirectory("${GENERAL_FUNCTIONS_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}/general-functions" EXCLUDE_FROM_ALL)
    else()
        find_package(GeneralFunctions CONFIG REQUIRED COMPONENTS lexer files geometry)
    endif()
endif()

if(TRIANGLES_BUILD_APP)
    add_executable(triangles
        main.cpp
        parser/parser.cpp
    )

    target_link_libraries(triangles PRIVATE
        GeneralFunctions::lexer
        GeneralFunctions::files
        GeneralFunctions::geometry)

    set_property(TARGET triangles PROPERTY CXX_SCAN_FOR_MODULES ON)

    target_include_directories(triangles PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/polygonProcessor"
    )

    triangles_configure_target(triangles)
endif()

option(TRIANGLES_BUILD_VISUALIZER "Build the visualizer window and backend factory" OFF)
option(TRIANGLES_BUILD_RENDER_TEST "Build the interactive lighting development demo" OFF)

if(TRIANGLES_BUILD_RENDER_TEST AND NOT TRIANGLES_BUILD_VISUALIZER)
    message(FATAL_ERROR "TRIANGLES_BUILD_RENDER_TEST requires TRIANGLES_BUILD_VISUALIZER")
endif()

if(TRIANGLES_BUILD_VISUALIZER)
    set(VISUALIZER_BUILD_EXAMPLES ${TRIANGLES_BUILD_RENDER_TEST})
    set(VISUALIZER_BUILD_TESTS ${BUILD_TESTING})
    add_subdirectory(visualizer)
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
        TRIANGLES_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    triangles_configure_target(parser_tests)
    gtest_discover_tests(parser_tests)
endif()
