# Development

Linux setup and validation:

    ./scripts/install-linux-prerequisites.sh
    ./scripts/validate.sh all

Vulkan presentation tests run through Xvfb and Mesa's software Vulkan implementation. The smoke path creates a hidden Vulkan window, presents three validation-clean frames and reports the adapter, swapchain extent, image count and format.

Windows CI compiles the presentation backend and runs hardware-independent tests without assuming that the hosted runner exposes a presentation-capable GPU.
