# Architecture

Afterlight separates application policy, platform integration, backend-independent rendering contracts, shader compilation and Vulkan execution.

    HLSL source
        |
        +--- DXC
                |
                +--- Vulkan 1.3 SPIR-V
                        |
                        +--- shader modules
                        +--- graphics pipeline
                        +--- dynamic rendering
                        +--- procedural draw

Shader compilation belongs to the CMake dependency graph. The application does not invoke the compiler at runtime. Generated binaries are validated before runtime presentation tests execute.

The first graphics pipeline owns its pipeline layout and Vulkan pipeline. It uses no descriptors and no vertex buffers. The vertex shader derives positions and colors from the vertex identifier, while the fragment shader writes interpolated color into the sRGB presentation attachment.

Swapchain recreation rebuilds the format-dependent graphics pipeline together with the presentation resources while preserving the device, command infrastructure, frame scheduler and backend-independent resource registry.
