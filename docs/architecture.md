# Architecture

Afterlight separates application policy, platform integration, deterministic scene construction, backend-independent rendering contracts, shader compilation and Vulkan execution.

    extruded observatory aperture
        |
        +--- vertex buffer
        +--- index buffer
        +--- camera transform
                |
                +--- color attachment
                +--- depth attachment
                        |
                        +--- dynamic rendering
                        +--- indexed draw

The depth target owns its Vulkan image, device-local memory and image view. Format selection prefers 32-bit floating-point depth, falls back to packed depth-stencil formats and is covered by a backend policy test.

The image begins undefined and receives one synchronization2 transition into depth-attachment layout before its first rendering scope. Swapchain recreation waits for device idleness, destroys format- and extent-dependent resources and creates a new depth target matching the presentation extent.

The scene mesh remains deterministic and independent of Vulkan. It contains front, rear, exterior and interior surfaces so depth testing has observable geometric work rather than merely satisfying pipeline configuration.
