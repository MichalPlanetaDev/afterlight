# Decisions

P13 makes texture coordinates part of the scene vertex contract.

Planar front and rear faces map their existing object-space aperture positions into zero-to-one coordinates. Inner and outer walls map segment boundaries across the horizontal texture interval and front-to-back depth across the vertical interval.

The final radial boundary remains one instead of wrapping to zero. Duplicated seam vertices therefore express both texture edges while retaining identical geometric positions.

The Vulkan vertex input uses location three with `R32G32_SFLOAT`. The shader consumes this attribute directly.

P13 does not alter aperture topology, material texture generation, descriptor ownership, GPU upload policy, depth ownership or lighting.
