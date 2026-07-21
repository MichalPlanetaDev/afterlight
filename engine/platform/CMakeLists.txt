find_package(SDL3 CONFIG REQUIRED)
find_package(volk CONFIG REQUIRED)

add_library(
    afterlight_platform
    STATIC
        src/platform.cpp
        src/vulkan_surface.cpp
)

add_library(Afterlight::Platform ALIAS afterlight_platform)

target_include_directories(
    afterlight_platform
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_link_libraries(
    afterlight_platform
    PUBLIC
        volk::volk_headers
    PRIVATE
        SDL3::SDL3
)

target_compile_definitions(
    afterlight_platform
    PUBLIC
        VK_NO_PROTOTYPES
)

afterlight_configure_target(afterlight_platform)
