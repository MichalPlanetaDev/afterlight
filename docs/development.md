# Development

Linux setup and validation:

    ./scripts/install-linux-prerequisites.sh
    ./scripts/validate.sh all

The vcpkg manifest supplies DXC on Linux and Windows. CMake compiles one HLSL source into separate Vulkan vertex and fragment SPIR-V binaries.

Validation checks the generated SPIR-V structure, builds the graphics pipeline under Clang, GCC and MSVC, runs sanitizer and static-analysis configurations, then creates and draws through the pipeline on Mesa's software Vulkan implementation with validation enabled.

For interactive development under WSLg, run:

    ./build/linux-clang-debug/apps/observatory/afterlight
