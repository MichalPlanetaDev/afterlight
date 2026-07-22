# Afterlight

[![CI](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml/badge.svg)](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml)

Afterlight is a native C++23 renderer developed around **The Last Observatory**, an interactive visual experience set inside a damaged orbital observatory reconstructing light from a vanished star.

The renderer compiles HLSL through Microsoft's DirectX Shader Compiler and emits Vulkan 1.3 SPIR-V during the CMake build. The Vulkan backend loads the generated stages, creates a dynamic-rendering graphics pipeline and submits the first procedural geometry without a vertex buffer.

Each frame transitions its swapchain image through explicit RHI states, clears the observatory background, draws a three-vertex gradient triangle and returns the image to presentation state.

Build and validate:

    ./scripts/install-linux-prerequisites.sh
    ./scripts/validate.sh all
    ./build/linux-clang-debug/apps/observatory/afterlight
