# Current State

Afterlight P10 is complete and locally validated.

The renderer version is `0.10.0-dev`. Vulkan renders the deterministic aperture from device-local vertex and index buffers populated through host-visible staging allocations, explicit transfer commands, synchronization2 barriers and fenced completion.

The preserved rendering contract contains 96 vertices, 144 indices, 96 flat normals, depth testing, directional lighting and two descriptor-backed frame-local scene uniform buffers.

Linux validation contains 18 passing tests. Repository history and the final synchronized `main` branch remain authoritative for the merged commit identity.
