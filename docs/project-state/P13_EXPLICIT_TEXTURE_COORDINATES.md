# P13 — Explicit Texture Coordinates

P13 replaces the shader-local position projection with a geometry-authored texture-coordinate contract.

Each aperture vertex owns one finite normalized coordinate. Front and rear faces use planar projection from aperture object space. Inner and outer walls use circumferential segment boundaries with depth mapped vertically.

The final segment terminates at one while the first begins at zero. Existing duplicated vertices make that radial seam explicit without changing topology or forcing interpolation across the full texture.

The Vulkan vertex layout exposes location three as `R32G32_SFLOAT`. HLSL reads the attribute directly.

P13_STATUS=EXPLICIT_TEXTURE_COORDINATES_VALIDATED
