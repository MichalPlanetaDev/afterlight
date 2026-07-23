# Current State

Afterlight P12 is complete and locally validated.

The renderer version is `0.12.0-dev`. The aperture samples a deterministic 64 by 64 project-owned material texture stored in an optimal-tiled device-local Vulkan image.

The generated sRGB colour channels describe the calibration surface and alpha carries bounded roughness. The exact payload checksum is `ad3a091625158275`.

Frame-local scene uniforms remain in descriptor set zero. The persistent material image and sampler occupy descriptor set one. Upload uses coherent-preferred host-visible staging memory, an explicit non-coherent flush fallback, synchronization2 image transitions, `vkCmdCopyBufferToImage` and fenced completion.

The preserved rendering contract contains 96 vertices, 144 indices, 96 flat normals, per-swapchain-image depth targets, directional lighting and two descriptor-backed scene-uniform frame buffers.

Linux validation contains 20 passing tests.
