# Development

Linux setup and validation:

    ./scripts/install-linux-prerequisites.sh
    ./scripts/validate.sh all

Interactive WSLg preview:

    ./build/linux-clang-debug/apps/observatory/afterlight

The native window renders the rotating observatory aperture with depth-correct occlusion and directional surface lighting. The spectral panels react continuously as their local normals rotate relative to the fixed world-space light.

Validation builds with Clang, GCC and MSVC, executes sanitizer and static-analysis configurations, verifies sixteen tests and submits three validation-clean lit frames through Mesa Vulkan.
