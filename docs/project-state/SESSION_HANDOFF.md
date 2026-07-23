# Session Handoff

Project: Afterlight / The Last Observatory

Repository: `MichalPlanetaDev/afterlight`

Local path: `~/projects/afterlight`

Completed phase: P14 — Persistent Material Parameter Block

Version: `0.14.0-dev`

Linux tests: 22/22

Current renderer: Vulkan dynamic rendering with explicit planar and radial texture coordinates, one depth target per swapchain image, device-local aperture mesh buffers, a deterministic sampled material texture, persistent aperture material parameters, flat normals, directional lighting and frame-local scene uniforms.

Geometry contract: 96 vertices, 144 indices, 96 normals and 96 texture coordinates

Material contract: descriptor set one, sampled image binding zero, sampler binding one, persistent 64-byte uniform buffer binding two

Texture contract: 64 by 64 RGBA8 sRGB, checksum `ad3a091625158275`, device-local optimal-tiled image

P15 has not started.

Completion marker: `P14_STATUS=PERSISTENT_MATERIAL_PARAMETER_BLOCK_VALIDATED`
