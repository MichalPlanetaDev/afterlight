# Development

Linux setup and validation:

    ./scripts/install-linux-prerequisites.sh
    ./scripts/validate.sh all

For an interactive WSLg preview:

    ./build/linux-clang-debug/apps/observatory/afterlight

The preview remains active until its native window is closed. Resizing the window recreates the swapchain and format-dependent graphics pipeline while preserving the indexed aperture mesh.

Validation builds the scene and Vulkan paths with Clang, GCC and MSVC, executes sanitizer and static-analysis configurations, verifies fourteen tests and submits three validation-clean indexed frames through Mesa Vulkan.
