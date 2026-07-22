# P10 — Device-Local Mesh Uploads

P10 moves the deterministic aperture vertex and index allocations into device-local Vulkan memory. Host-visible staging allocations receive the CPU-authored geometry, prefer coherent memory and retain an explicit non-coherent flush fallback.

A one-time graphics command buffer records both copies and synchronization2 barriers for vertex-attribute and index visibility. Fence completion protects destruction of staging resources and the transient command pool.

The milestone preserves 96 vertices, 144 indices, 96 explicit flat normals, depth testing, directional lighting and two descriptor-backed frame-local scene uniform buffers.

P10_STATUS=DEVICE_LOCAL_MESH_UPLOADS_VALIDATED
