# Current State

Afterlight P13 is complete and locally validated.

The renderer version is `0.13.0-dev`. Every aperture vertex now owns an explicit two-component texture coordinate generated with its surface geometry.

Front and rear faces use deterministic planar projection. Inner and outer radial walls use circumferential segment coordinates with depth mapped vertically. The duplicated radial seam carries zero at the first boundary and one at the final boundary without modifying geometry or index topology.

The Vulkan vertex layout exposes the coordinate as `R32G32_SFLOAT` at location three. HLSL consumes the declared attribute directly and no longer derives material coordinates from object position.

The preserved rendering contract contains 96 vertices, 144 indices, 96 flat normals, 96 texture coordinates, a device-local procedural material texture, per-swapchain-image depth targets, directional lighting and two frame-local scene-uniform descriptor sets.

Linux validation contains 21 passing tests.
