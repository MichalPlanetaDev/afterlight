# Development

Afterlight uses a pinned vcpkg manifest, CMake presets and mandatory
pull-request validation.

Linux setup:

```bash
./scripts/install-linux-prerequisites.sh
./scripts/bootstrap-vcpkg.sh
```

Windows setup:

```powershell
./scripts/bootstrap-vcpkg.ps1
```

The normal Linux validation matrix is:

```bash
cmake --preset linux-clang-debug
cmake --build --preset linux-clang-debug
ctest --preset linux-clang-debug

cmake --preset linux-clang-sanitize
cmake --build --preset linux-clang-sanitize
ctest --preset linux-clang-sanitize

cmake --preset linux-clang-analysis
cmake --build --preset linux-clang-analysis

cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
```

Normal execution creates the native resizable window. `--smoke` selects
SDL's dummy video backend and exercises initialization, window creation,
event polling and shutdown without a display server.

Project-owned code is formatted before commit and compiled with warnings as
errors. Changes enter `main` only after Linux and Windows checks pass.
