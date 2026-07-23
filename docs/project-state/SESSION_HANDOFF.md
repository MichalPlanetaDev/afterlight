# Session Handoff

Project: Afterlight / The Last Observatory

Repository: `MichalPlanetaDev/afterlight`

Local path: `~/projects/afterlight`

Completed phase: P13 — Explicit Texture Coordinates

Version: `0.13.0-dev`

Linux tests: 21/21

Current renderer: Vulkan dynamic rendering with explicit planar and radial texture coordinates, one depth target per swapchain image, device-local aperture mesh buffers, a deterministic sampled material texture, flat normals, directional lighting and frame-local scene uniforms.

Geometry contract: 96 vertices, 144 indices, 96 normals and 96 texture coordinates

Mapping contract: planar front and rear faces; circumferential inner and outer walls; duplicated zero-to-one radial seam

P14 has not started.

Completion marker: `P13_STATUS=EXPLICIT_TEXTURE_COORDINATES_VALIDATED`
