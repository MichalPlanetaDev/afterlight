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

## P12 material validation

The P12 unit contract verifies deterministic RGBA8 generation, the versioned FNV-1a checksum, semantic channel values, extent limits, ring-parameter validation and coherent-preferred memory selection with a non-coherent fallback.

The Vulkan smoke path requires a 64 by 64 procedural calibration texture, device-local image memory, descriptor set one bindings and checksum `ad3a091625158275`. Full validation contains twenty tests and preserves the P11 geometry, depth, lighting and scene-uniform contracts.

## P13 texture-coordinate validation

The scene contract verifies deterministic generation, finite normalized coordinates, planar mapping on front and rear surfaces, radial mapping on both wall classes and an explicit zero-to-one seam across duplicated geometry.

The Vulkan smoke path requires ninety-six explicit coordinates and the `explicit-planar-radial` mapping marker. Full Linux validation contains twenty-one tests and preserves every P12 texture, descriptor, geometry, depth and lighting contract.
