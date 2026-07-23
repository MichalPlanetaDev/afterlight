# Development

Linux setup and validation:

    ./scripts/install-linux-prerequisites.sh
    ./scripts/validate.sh all

Interactive WSLg preview:

    ./build/linux-clang-debug/apps/observatory/afterlight

The native window renders the depth-tested, directionally lit observatory aperture while updating descriptor-backed scene data independently for each frame in flight.

Validation builds with Clang, GCC and MSVC, executes sanitizer and static-analysis configurations, verifies nineteen tests and submits three validation-clean frames using Vulkan uniform buffers and descriptor sets.

## P10 validation

The Vulkan smoke contract reports `mesh-memory=device-local` while preserving 96 vertices, 144 indices, 96 normals, depth testing, directional lighting and two descriptor-backed scene-uniform frames.

## P11 validation

The Vulkan smoke contract reports `depth-ownership=per-swapchain-image`. Validation also requires the reported depth-target count to equal the runtime swapchain-image count while preserving the established geometry, lighting, device-local mesh and descriptor-uniform contracts.
