# Afterlight

[![CI](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml/badge.svg)](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml)

Afterlight is a native C++23 renderer developed around **The Last
Observatory**, an interactive visual experience set inside a damaged
orbital observatory reconstructing light from a vanished star.

The project now owns a real cross-platform application boundary. SDL3
initializes the desktop video subsystem, creates the resizable native
window and translates operating-system events into a small project
event model. The runtime still renders no scene; the next layer is the
Vulkan instance, presentation surface and first validated frame.

## Build

Dependencies are pinned through the repository's vcpkg manifest.

```bash
./scripts/bootstrap-vcpkg.sh
cmake --preset linux-clang-debug
cmake --build --preset linux-clang-debug
ctest --preset linux-clang-debug
```

Windows uses `scripts/bootstrap-vcpkg.ps1` followed by the
`windows-msvc-debug` presets.

## Run

```bash
./build/linux-clang-debug/apps/observatory/afterlight
```

The application opens a resizable 1280x720 desktop window and exits
through the explicit lifecycle when the window is closed. CI uses
`--smoke`, which selects SDL's dummy video backend and exercises the
same creation and shutdown path without requiring a display server.

## Structure

```text
apps/observatory/    Application composition and main loop
engine/core/         Lifecycle and immutable build identity
engine/platform/     SDL ownership, window and event translation
tests/               Unit and headless integration coverage
cmake/               Compiler and analysis policy
```

Vulkan is implemented next. Direct3D 12 follows only after the resource,
synchronization and command-submission contracts have been validated
against the Vulkan backend.
