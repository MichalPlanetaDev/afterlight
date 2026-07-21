# Development

Work is performed on focused branches and enters `main` through pull
requests. Each branch must build locally before publication; GitHub
Actions repeats the supported compiler and platform matrix.

The normal Linux validation path is:

```bash
cmake --preset linux-clang-debug
cmake --build --preset linux-clang-debug
ctest --preset linux-clang-debug

cmake --preset linux-clang-sanitize
cmake --build --preset linux-clang-sanitize
ctest --preset linux-clang-sanitize

cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
```

Project-owned code is compiled with warnings as errors. Production
behavior is introduced behind a failing test when the behavior can be
tested independently. Commits remain narrow enough to review without
mixing unrelated architecture, formatting and feature changes.

Generated build directories are local. `CMakeUserPresets.json` may be
used for machine-specific compiler launchers or paths and is intentionally
excluded from version control.
