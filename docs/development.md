# Development

Linux setup and validation:

    ./scripts/install-linux-prerequisites.sh
    ./scripts/validate.sh all

Interactive WSLg preview:

    ./build/linux-clang-debug/apps/observatory/afterlight

The native window renders the depth-tested, directionally lit observatory aperture while updating descriptor-backed scene data independently for each frame in flight.

Validation builds with Clang, GCC and MSVC, executes sanitizer and static-analysis configurations, verifies seventeen tests and submits three validation-clean frames using Vulkan uniform buffers and descriptor sets.
