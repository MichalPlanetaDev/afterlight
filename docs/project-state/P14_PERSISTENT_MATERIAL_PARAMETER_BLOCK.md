# P14 — Persistent Material Parameter Block

P14 moves stable aperture surface response from fragment-shader literals into one repository-owned persistent material parameter contract.

The host representation occupies 64 bytes with sixteen-byte alignment and maps directly to four HLSL `float4` registers. Defaults preserve the P13 rendered response exactly.

Material descriptor set one retains sampled image binding zero and sampler binding one. A persistent fragment-stage uniform buffer occupies binding two.

The allocation uses host-visible memory, prefers coherence and flushes explicitly when the compatible memory type is non-coherent. It is initialized once and survives swapchain recreation with the material texture.

Frame-local scene uniforms remain separate.

P14_STATUS=PERSISTENT_MATERIAL_PARAMETER_BLOCK_VALIDATED
