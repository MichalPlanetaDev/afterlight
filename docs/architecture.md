# Architecture

Afterlight separates application policy, platform integration, scene data, backend-independent rendering contracts, shader compilation and Vulkan execution.

    deterministic aperture mesh
        |
        +--- host-visible vertex buffer
        +--- host-visible index buffer
                |
                +--- Vulkan binding
                +--- indexed draw

    model transform
        |
        +--- view transform
                |
                +--- Vulkan perspective projection
                        |
                        +--- push constants
                        +--- HLSL vertex stage

The scene module owns deterministic CPU geometry and camera mathematics without exposing GLM types through its public interface. The Vulkan backend owns buffer allocation, memory selection, mapping, synchronization and draw recording.

The current upload path uses host-visible memory because the aperture is small and immutable. A later asset-streaming milestone can introduce staging buffers and device-local allocation without changing scene mesh data.

Swapchain recreation rebuilds the format-dependent graphics pipeline while retaining the GPU mesh, camera state, command infrastructure, frame scheduler and backend-independent resource registry.
