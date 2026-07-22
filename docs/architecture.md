# Architecture

Afterlight separates application policy, platform integration, deterministic scene construction, backend-independent rendering contracts, shader compilation and Vulkan execution.

    deterministic mesh
        |
        +--- position
        +--- flat surface normal
        +--- spectral base color
                |
                +--- indexed GPU buffers
                +--- depth-tested pipeline
                +--- directional lighting

The aperture uses unique vertices for each logical surface. This increases the mesh from twenty-four shared vertices to ninety-six face vertices while preserving forty-eight indexed triangles. Front, rear, exterior and interior surfaces therefore carry physically meaningful flat normals without smoothing across hard mechanical edges.

The scene frame block contains the model-view-projection transform, an object-space light direction with intensity and an object-space view direction with exposure. Its total size is ninety-six bytes, remaining within Vulkan's guaranteed minimum push-constant capacity. The range is visible to both vertex and fragment shader stages.

The fragment stage operates in linear color. It combines hemispherical ambient response, Lambertian diffuse illumination, a compact specular lobe and a low-energy rim term before applying exponential exposure mapping. The sRGB swapchain performs the final transfer conversion.
