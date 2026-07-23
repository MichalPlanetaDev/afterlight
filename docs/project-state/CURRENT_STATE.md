# Current State

Afterlight P14 is complete and locally validated.

The renderer version is `0.14.0-dev`. The persistent aperture material now owns a 64-byte, sixteen-byte-aligned parameter block in addition to its procedural sampled image and sampler.

Material descriptor set one retains image binding zero and sampler binding one. Binding two is a fragment-stage uniform buffer containing calibrated surface response, roughness-dependent specular response, rim response and neutral specular tint.

The parameter buffer uses host-visible memory with coherent preference and explicit non-coherent flushing. It is initialized once and survives swapchain recreation with the material texture.

HLSL consumes the same four-register contract. Frame-local transforms, lighting vectors, view direction and exposure remain isolated in scene descriptor set zero.

The preserved rendering contract contains 96 vertices, 144 indices, 96 normals, 96 explicit texture coordinates, the versioned 64 by 64 procedural texture with checksum `ad3a091625158275`, per-swapchain-image depth targets and two frame-local scene-uniform descriptor sets.

Linux validation contains 22 passing tests.
