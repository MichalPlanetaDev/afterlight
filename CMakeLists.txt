cmake_minimum_required(VERSION 3.28)

project(
    Afterlight
    VERSION 0.6.0
    DESCRIPTION "Native renderer and interactive visual experience"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(AfterlightOptions)
include(AfterlightWarnings)
include(AfterlightSanitizers)
include(AfterlightTarget)

afterlight_configure_project_analysis()

include(CTest)

add_subdirectory(shaders)
add_subdirectory(engine/core)
add_subdirectory(engine/rhi)
add_subdirectory(engine/platform)
add_subdirectory(engine/scene)
add_subdirectory(engine/graphics/vulkan)
add_subdirectory(apps/observatory)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
