# Development

Linux setup and validation:

    ./scripts/install-linux-prerequisites.sh
    ./scripts/validate.sh all

The validation matrix covers resource descriptor rejection, generational handle invalidation, slot reuse, frame scheduling, resource-state transitions and Vulkan barrier translation. Runtime presentation still executes through Xvfb and Mesa's software Vulkan implementation.

The RHI remains independent of Vulkan headers. Vulkan-specific translation belongs to the backend and is tested separately from the backend-neutral state model.
