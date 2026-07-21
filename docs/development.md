# Development

Linux setup and the complete local validation matrix use repository-owned scripts:

```bash
./scripts/install-linux-prerequisites.sh
./scripts/validate.sh all
```

Runtime Vulkan tests execute through Xvfb with Mesa's software Vulkan implementation, making device and presentation-surface validation reproducible without a physical display. Windows CI compiles the complete Vulkan backend and runs hardware-independent tests without assuming a GPU is available on the runner.

Full dependency, compiler, static-analysis and test output is retained under `out/logs/`; the terminal shows only phase status and failure diagnostics.
