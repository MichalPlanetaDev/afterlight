# Afterlight

[![CI](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml/badge.svg)](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml)

Afterlight is a native C++23 renderer developed around **The Last Observatory**, an interactive visual experience set inside a damaged orbital observatory reconstructing light from a vanished star.

The renderer now has a project-owned rendering hardware interface foundation. Generational texture handles prevent stale resource access, immutable descriptors define imported presentation images, explicit resource states drive backend barriers, and a frame scheduler controls recording, submission and completion across the two frames in flight.

The Vulkan presentation path consumes these contracts directly. Swapchain images are imported into the resource registry, tracked from undefined state through color attachment usage and presentation, then translated into synchronization2 image barriers by the Vulkan backend.

Build and validate:

    ./scripts/install-linux-prerequisites.sh
    ./scripts/validate.sh all
    ./build/linux-clang-debug/apps/observatory/afterlight

The next milestone introduces HLSL compilation, pipeline layouts and the first generated geometry.
