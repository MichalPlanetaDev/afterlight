# Architecture

Afterlight separates application policy, platform integration and graphics ownership.

    observatory application
            |
            +--- core lifecycle
            |
            +--- SDL3 platform
            |       |
            |       +--- native window
            |       +--- Vulkan surface bridge
            |
            +--- Vulkan backend
                    |
                    +--- instance and validation
                    +--- physical and logical device
                    +--- swapchain and image views
                    +--- frame synchronization
                    +--- dynamic rendering

The platform layer owns SDL and native-window lifetime. The Vulkan backend owns every Vulkan object from instance creation through presentation.

Two frame contexts hold command buffers, acquisition semaphores and submission fences. Render-completion semaphores are owned per swapchain image so they are not reused before presentation releases them. Acquired images also retain the fence of their latest graphics submission.

Swapchain recreation waits for device idleness and replaces only presentation-dependent resources. The instance, surface, physical device, logical device, queues, command pool and frame contexts remain intact.
