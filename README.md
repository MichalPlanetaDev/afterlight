# Afterlight

[![CI](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml/badge.svg)](https://github.com/MichalPlanetaDev/afterlight/actions/workflows/ci.yml)

Afterlight is a native C++23 rendering project built around **The Last
Observatory**, an interactive visual experience set inside a damaged
orbital observatory reconstructing light from a vanished star.

This is not an engine wrapper or a model viewer. The project is being
developed as a renderer with its own runtime boundaries, graphics
abstraction and diagnostic tooling. Vulkan is the first graphics
backend; Direct3D 12 follows after the rendering interface and resource
model are stable.

## Current state

The repository currently contains the portable runtime foundation rather
than the renderer itself. It provides a tested C++23 build, explicit
application lifecycle, strict compiler policy and cross-platform CI.
Renderer, platform and graphics modules will be introduced in subsequent
pull requests without hiding incomplete systems behind placeholder
abstractions.

The executable currently validates startup, transition into the run state,
a controlled stop request and deterministic shutdown:

```text
Afterlight 0.0.0-dev | Milestone Zero | local
```

## Build

Linux with Clang:

```bash
cmake --preset linux-clang-debug
cmake --build --preset linux-clang-debug
ctest --preset linux-clang-debug
```

Linux with GCC uses `linux-gcc-debug`. The sanitizer configuration is
available as `linux-clang-sanitize`.

Windows with Visual Studio 2022:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
```

CMake 3.28 or newer is required.

## Repository structure

```text
apps/observatory/    Native application entry point
engine/core/         Runtime-independent foundation
tests/unit/          Focused behavior tests
cmake/               Compiler, warning and sanitizer policy
docs/architecture.md Current and planned system boundaries
docs/development.md  Local workflow and validation commands
```

Afterlight is developed publicly through focused branches, pull requests
and GitHub Actions. The repository documents systems that exist or have a
committed implementation path; it does not present speculative features
as completed work.
