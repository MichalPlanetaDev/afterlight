find_package(volk CONFIG REQUIRED)

add_library(
    afterlight_vulkan
    STATIC
        src/barrier.cpp
        src/device_selection.cpp
        src/gpu_mesh.cpp
        src/mesh_pipeline.cpp
        src/swapchain.cpp
        src/vulkan_context.cpp
)

add_library(Afterlight::Vulkan ALIAS afterlight_vulkan)

target_include_directories(
    afterlight_vulkan
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_link_libraries(
    afterlight_vulkan
    PUBLIC
        Afterlight::Platform
        Afterlight::Rhi
        Afterlight::Scene
        volk::volk_headers
    PRIVATE
        volk::volk
)

target_compile_definitions(
    afterlight_vulkan
    PUBLIC
        VK_NO_PROTOTYPES
)

afterlight_configure_target(afterlight_vulkan)
