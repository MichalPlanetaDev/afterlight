# ADR 0014: Persistent aperture material parameter block

## Context

P12 established persistent sampled-image and sampler ownership in material descriptor set one. P13 established explicit geometry-authored sampling coordinates. The fragment shader still owned the remaining aperture surface response through embedded numeric literals, leaving no host-side material schema and no descriptor-backed parameter contract.

Those controls change with material identity rather than with frame cadence. Moving them into the frame-local scene uniform block would combine unrelated lifetimes and duplicate immutable aperture state for every frame in flight.

## Decision

`ApertureMaterialParameters` is a 64-byte, sixteen-byte-aligned host representation composed of four four-component registers. It owns the calibrated surface response, roughness-dependent specular response, rim response and neutral specular tint used by the aperture material.

The defaults reproduce the P13 shader constants exactly. Validation rejects non-finite values, inverted response ranges, unsupported bounds and non-zero reserved register components.

`MaterialTexture` continues to own persistent material descriptor set one. Image binding zero and sampler binding one remain unchanged. Binding two is one fragment-stage uniform buffer containing the immutable aperture material parameter block.

The parameter buffer uses host-visible memory with coherent preference and explicit non-coherent flushing. It is written once during material construction and survives swapchain recreation with the sampled image and sampler.

The HLSL contract contains the same four-register field order and consumes the persistent material buffer directly. Camera transforms, light direction, light intensity, view direction and exposure remain in frame-local scene descriptor set zero.

## Consequences

Material response is now repository-owned, bounded and testable on the host while retaining the established visual defaults.

The material descriptor layout contains three bindings. The resource remains immutable after construction, and no per-frame material allocation or update path is introduced.

P14 does not introduce a generic material graph, multiple material instances, runtime editing, tangent space, normal maps, imported assets or mip generation.
