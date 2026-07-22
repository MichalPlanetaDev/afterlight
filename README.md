# Afterlight

[![CI](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml/badge.svg)](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml)

Afterlight is a native C++23 renderer developed around **The Last Observatory**, an interactive visual experience set inside a damaged orbital observatory reconstructing light from a vanished star.

The observatory aperture is now an extruded three-dimensional indexed mesh with separate front, rear, outer and inner surfaces. A device-local Vulkan depth image is recreated with the swapchain, transitioned through synchronization2 and attached to dynamic rendering.

The graphics pipeline enables depth testing and depth writes against a Vulkan zero-to-one projection. As the aperture rotates, its front, rear and internal surfaces occlude correctly instead of relying on submission order.

Build and validate:

    ./scripts/install-linux-prerequisites.sh
    ./scripts/validate.sh all
    ./build/linux-clang-debug/apps/observatory/afterlight
