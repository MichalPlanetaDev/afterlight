# Development

Linux setup and validation:

    ./scripts/install-linux-prerequisites.sh
    ./scripts/validate.sh all

Interactive WSLg preview:

    ./build/linux-clang-debug/apps/observatory/afterlight

The native window renders an extruded rotating observatory aperture with depth-correct occlusion. Resizing recreates the swapchain, depth image and graphics pipeline while preserving the GPU mesh and frame infrastructure.

Validation builds with Clang, GCC and MSVC, executes sanitizer and static-analysis configurations, verifies fifteen tests and submits three validation-clean depth-buffered frames through Mesa Vulkan.
