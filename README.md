# Afterlight

[![CI](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml/badge.svg)](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml)

Afterlight is a native C++23 renderer developed around **The Last Observatory**, an interactive visual experience set inside a damaged orbital observatory reconstructing light from a vanished star.

The renderer maintains one persistently mapped scene uniform buffer for each frame in flight. A descriptor set at set zero and binding zero exposes the model-view-projection transform, directional-light parameters and camera-dependent exposure data to both HLSL shader stages.

The memory policy prefers host-visible coherent memory and falls back to explicit non-coherent flushing when required. Each frame slot waits for its fence before the CPU updates its corresponding allocation, preventing writes from racing GPU reads.

Build and validate:

    ./scripts/install-linux-prerequisites.sh
    ./scripts/validate.sh all
    ./build/linux-clang-debug/apps/observatory/afterlight

## Device-local mesh uploads

The Vulkan backend stores deterministic aperture vertices and indices in device-local buffers. Host-visible staging allocations provide the CPU transfer path, coherent memory is preferred and non-coherent staging memory uses an explicit mapped-memory flush. A fenced one-time submission protects staging lifetime and makes transfer writes visible to vertex and index reads.

## Per-image depth ownership

Every swapchain image owns a matching device-local depth target. The acquired image index selects both attachments, and the existing image fence protects their reuse. This removes writable depth sharing between independently in-flight frames.

## Procedural material texture

The aperture now samples a project-owned calibration surface generated deterministically with integer mathematics. Its sRGB colour channels describe restrained steel, alloy and optical accents while alpha carries bounded roughness. The immutable image resides in device-local memory and is uploaded through a fenced staging transfer before rendering begins.

Scene uniforms remain frame-local in descriptor set zero. Persistent material image and sampler state occupy descriptor set one, keeping ownership and update frequency explicit.

## Explicit aperture texture coordinates

The aperture generator now authors texture coordinates with the geometry instead of reconstructing them in HLSL. Front and rear faces use deterministic planar projection, while inner and outer walls use a circumferential mapping with an explicit duplicated seam. The Vulkan vertex layout and shader interface consume the same two-component attribute contract.

## Persistent aperture material parameters

The aperture material now carries a repository-owned 64-byte parameter block beside its sampled image and sampler. The block preserves the established surface response while moving stable shader literals into one persistent descriptor-backed contract with explicit host and HLSL layout agreement.
