# Decisions

P14 makes stable aperture surface response explicit persistent material state.

`ApertureMaterialParameters` occupies four sixteen-byte registers and preserves the P13 visual constants as validated defaults. Unsupported ranges, non-finite values, inverted response relationships and non-zero reserved components are rejected.

Material descriptor set one retains sampled image binding zero and sampler binding one. A persistent uniform buffer occupies binding two.

Material parameters follow material lifetime and are not duplicated per frame. Scene transforms, light direction, light intensity, view direction and exposure remain frame-local in descriptor set zero.

P14 does not add runtime material editing, multiple material instances, a generic material framework, tangent space, normal mapping, imported assets or mip generation.
