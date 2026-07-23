# ADR 0013: Explicit aperture texture coordinates

## Context

P12 established a persistent sampled material texture, but the vertex shader generated sampling coordinates from object-space position. That shortcut coupled material placement to the shader and could not express different mapping requirements for planar faces, outer walls and inner walls.

The aperture generator already duplicates vertices for flat shading and surface-specific normals. Those duplicated vertices provide the correct ownership boundary for equally surface-specific texture coordinates.

## Decision

The scene vertex contract now contains an explicit two-component texture coordinate. The deterministic aperture generator writes one coordinate for every generated vertex.

Front and rear surfaces use planar object-space projection. Their aperture positions in the range minus one to one map directly into the normalized zero-to-one texture domain.

Outer and inner walls use circumferential coordinates. Each segment occupies one sixth of the horizontal texture interval, while front and rear depths occupy the vertical endpoints. The final radial boundary remains one rather than wrapping to zero. Because seam vertices are already duplicated, the same geometric boundary can legally carry zero on the first segment and one on the final segment without interpolation across the complete texture.

The Vulkan vertex-input declaration exposes location three as `R32G32_SFLOAT`. HLSL consumes that attribute directly and no longer derives material coordinates from position.

## Consequences

Texture placement is now authored by geometry generation rather than hidden inside a shader expression. Surface roles, radial continuity and seam behavior are testable before GPU upload.

The vertex stride increases by two floats. Vertex and index counts, flat normals, device-local upload, sampled material ownership, depth ownership and lighting remain unchanged.

P13 does not introduce arbitrary mesh import, tangent space, normal mapping, material parameters, mip generation or general-purpose unwrap tooling.
