# Architecture

Afterlight separates application policy, platform integration and graphics ownership.

```text
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
                +--- physical-device policy
                +--- logical device and queues
```

The platform layer owns SDL types and native-window lifetime. Its Vulkan bridge exposes only the instance extensions and surface operations required by the graphics backend.

The Vulkan backend owns loader initialization, instance lifetime, validation messaging, surface lifetime, adapter qualification and logical-device creation. Adapter policy is isolated from Vulkan handles and tested with synthetic capability descriptions.

The next boundary adds swapchain ownership, frame contexts, command buffers, semaphores, fences and resize-safe presentation.
