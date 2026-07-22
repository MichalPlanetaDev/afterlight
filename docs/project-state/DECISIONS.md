# Decisions

P10 uses host-visible staging allocations and device-local final mesh allocations. Host-coherent staging memory is preferred, while non-coherent staging memory uses an explicit mapped-memory flush.

The current upload is synchronous and fenced because the renderer contains one bounded static aperture mesh. It is not the final asynchronous asset-streaming architecture.

Transfer writes become visible to vertex-attribute and index reads through synchronization2 buffer barriers recorded in the upload command buffer.

P09 descriptor-backed frame uniforms, depth testing, explicit flat normals and directional lighting remain unchanged.
