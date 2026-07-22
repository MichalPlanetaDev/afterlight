# Afterlight

[![CI](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml/badge.svg)](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml)

Afterlight is a native C++23 renderer developed around **The Last Observatory**, an interactive visual experience set inside a damaged orbital observatory reconstructing light from a vanished star.

The Vulkan backend now renders deterministic indexed mesh data from project-owned GPU vertex and index buffers. A concept-specific observatory aperture replaces the earlier procedural triangle and rotates under a perspective camera built with GLM.

Camera matrices are converted into an explicit row representation and delivered through Vulkan push constants. The HLSL vertex stage applies the model, view and Vulkan zero-to-one projection transform before the fragment stage resolves the aperture's spectral color gradient.

Build and validate:

    ./scripts/install-linux-prerequisites.sh
    ./scripts/validate.sh all
    ./build/linux-clang-debug/apps/observatory/afterlight
