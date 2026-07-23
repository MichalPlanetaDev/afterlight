# P12 — Procedural Material Texture

P12 establishes the first persistent sampled-image path.

A deterministic integer generator creates a 64 by 64 aperture calibration surface with concentric divisions, radial structure, an optical hub and a roughness channel. Its versioned FNV-1a checksum is `ad3a091625158275`.

The generated payload is copied from coherent-preferred host-visible staging memory into an optimal-tiled device-local sRGB image. Non-coherent staging memory is flushed explicitly. Synchronization2 transitions the image into transfer-destination and shader-read-only layouts, and a fence completes upload before staging destruction.

`MaterialTexture` owns the Vulkan image, memory, view, sampler and persistent descriptor resources. Scene uniforms remain in set zero. The material image and sampler use set one, bindings zero and one.

P12_STATUS=PROCEDURAL_MATERIAL_TEXTURE_VALIDATED
