# Architecture

Afterlight separates application policy, platform integration, backend-independent rendering contracts and Vulkan implementation.

    observatory application
            |
            +--- core lifecycle
            |
            +--- SDL3 platform
            |       |
            |       +--- native window
            |       +--- Vulkan surface bridge
            |
            +--- RHI
            |       |
            |       +--- generational resource handles
            |       +--- immutable resource descriptors
            |       +--- explicit resource states
            |       +--- frame scheduling
            |
            +--- Vulkan backend
                    |
                    +--- instance and device
                    +--- swapchain and image views
                    +--- synchronization2 translation
                    +--- dynamic rendering
                    +--- presentation

The RHI does not hide Vulkan concepts behind a lowest-common-denominator API. It establishes ownership, identity, state and scheduling contracts that can later be implemented by both Vulkan and Direct3D 12.

Swapchain images are external texture resources. Each recreation invalidates their previous generational handles, imports the new images and initializes their tracked state as undefined. Command recording requests state transitions through the RHI tracker, while the Vulkan backend translates those transitions into stage masks, access masks and image layouts.

Frame slots move through available, recording and submitted states. A slot becomes available only after its submission fence has completed. Device-idle swapchain recreation resets the frame scheduler without reconstructing the device or command infrastructure.
