# Afterlight

[![CI](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml/badge.svg)](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml)

Afterlight is a native C++23 renderer developed around **The Last Observatory**, an interactive visual experience set inside a damaged orbital observatory reconstructing light from a vanished star.

The Vulkan backend now owns surface negotiation, swapchain images and views, command buffers, image acquisition, per-image render-completion semaphores, fences, synchronization2 barriers and Vulkan 1.3 dynamic rendering. Each frame clears a swapchain attachment and presents it through the selected graphics and presentation queues. Resize and out-of-date results rebuild presentation-dependent resources without rebuilding the Vulkan device.

Build and validate:

    ./scripts/install-linux-prerequisites.sh
    ./scripts/validate.sh all
    ./build/linux-clang-debug/apps/observatory/afterlight

The next milestone introduces shader compilation, pipeline layouts and generated geometry.
