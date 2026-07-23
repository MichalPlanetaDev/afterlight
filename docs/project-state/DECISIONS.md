# Decisions

P12 uses a dedicated persistent `MaterialTexture` resource.

The object owns one device-local optimal-tiled image, its memory, image view, sampler and descriptor resources. It survives swapchain recreation and remains immutable after its upload fence completes.

Frame-local scene data stays in descriptor set zero. Persistent material state uses descriptor set one, with the sampled image at binding zero and sampler at binding one.

The first surface is generated with integer mathematics from versioned dimensions and ring parameters. It does not rely on imported assets or decorative random noise.

RGB is stored as sRGB colour data. Alpha stores bounded roughness. Linear filtering, clamp-to-edge addressing, one mip level and disabled anisotropy are explicit current policies.

P12 does not introduce an asset compiler, imported texture formats, compression, automatic mip generation, material instances or general-purpose texture management.
