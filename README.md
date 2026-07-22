# Afterlight

[![CI](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml/badge.svg)](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml)

Afterlight is a native C++23 renderer developed around **The Last Observatory**, an interactive visual experience set inside a damaged orbital observatory reconstructing light from a vanished star.

The observatory aperture now carries explicit flat-shaded surface normals. Separate vertices preserve the front, rear, exterior and interior face directions required for stable lighting across the extruded structure.

A world-space directional key light and view direction are transformed into object space for each frame, packed with the model-view-projection matrix and delivered through a 96-byte Vulkan push-constant block visible to both shader stages. The HLSL fragment stage combines hemispherical ambient light, Lambertian diffuse response, restrained specular reflection, rim lighting and exposure mapping.

Build and validate:

    ./scripts/install-linux-prerequisites.sh
    ./scripts/validate.sh all
    ./build/linux-clang-debug/apps/observatory/afterlight
