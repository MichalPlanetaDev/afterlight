# Architecture

Afterlight separates application policy, platform integration, deterministic scene construction, backend-independent rendering contracts, shader compilation and Vulkan execution.

    scene frame data
        |
        +--- frame slot zero uniform buffer
        +--- frame slot one uniform buffer
                |
                +--- descriptor pool
                +--- descriptor sets
                +--- pipeline layout set zero
                        |
                        +--- vertex shader
                        +--- fragment shader

The scene-uniform subsystem owns a descriptor-set layout, descriptor pool, one persistently mapped uniform buffer per frame in flight and one descriptor set per buffer. The subsystem survives swapchain recreation because its data shape is independent of surface extent and attachment formats.

The renderer waits for a frame slot's fence before writing its uniform allocation. Coherent memory requires no additional operation. A non-coherent fallback issues a mapped-memory flush before command submission.

The graphics pipeline consumes the uniform descriptor layout instead of declaring a large push-constant range. This leaves push constants available for future draw-local data while establishing the descriptor infrastructure required by material textures, samplers and larger scene resources.

## Device-local static geometry

Static aperture geometry crosses an explicit staging boundary. CPU-authored data enters host-visible transfer-source buffers and is copied into device-local vertex and index destinations. Synchronization2 barriers establish vertex-input visibility, while fence completion protects staging and command-pool destruction. Frame-local descriptor-backed scene uniforms remain a separate lifetime domain.

## Per-swapchain-image depth targets

The swapchain owns one depth target per presentation image. Command recording selects the depth attachment with the acquired image index, so the image-specific fence protects color and depth reuse as one ownership unit. Resize recreation destroys and rebuilds the complete attachment set.

## Persistent material binding

`MaterialTexture` is a cohesive Vulkan lifetime owner for the aperture's generated surface. It owns the device-local image allocation, image view, sampler and persistent descriptor resources. The object is created once with the renderer and survives swapchain recreation because neither its extent nor shader binding depends on presentation state.

The scene descriptor set remains set zero and contains frame-local uniform data. The material descriptor set is set one, with the sampled image at binding zero and sampler at binding one. The mesh pipeline declares both layouts and binds both sets in one command.

## Surface-aware texture-coordinate contract

`scene::Vertex` owns the texture coordinate used by the material shader. Planar aperture faces map object-space positions into the normalized texture domain. Radial walls map segment boundaries across the horizontal interval and depth across the vertical interval.

The generator retains one at the final radial boundary rather than applying modulo arithmetic. Existing duplicated seam vertices therefore represent the same position with zero and one coordinates without introducing an interpolation discontinuity through unrelated texture regions.

## Persistent material parameter lifetime

Material descriptor set one owns resources that survive swapchain recreation. Binding zero remains the sampled aperture image, binding one remains its sampler, and binding two contains the immutable 64-byte aperture material parameter block.

The material block is deliberately separate from scene descriptor set zero. Scene transforms, lighting vectors, exposure and frame cadence remain frame-local, while surface response follows material lifetime.
