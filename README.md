# Afterlight

[![CI](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml/badge.svg)](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml)

Afterlight is a native C++23 renderer developed around **The Last Observatory**, an interactive visual experience set inside a damaged orbital observatory reconstructing light from a vanished star.

The runtime owns the native SDL3 window, Vulkan loader, Vulkan 1.3 instance, validation callback, presentation surface, physical-device selection, logical device and graphics/presentation queues. Adapter selection requires surface compatibility and `VK_KHR_swapchain`, while preserving separate graphics and presentation queue families when necessary.

```bash
./scripts/install-linux-prerequisites.sh
./scripts/validate.sh all
./build/linux-clang-debug/apps/observatory/afterlight
```

The executable reports the selected Vulkan device and remains active until the native window closes. The next milestone creates the swapchain, frame synchronization and first presented clear frame.
