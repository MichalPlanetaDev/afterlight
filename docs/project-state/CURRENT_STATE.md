# Current State

Afterlight P11 is complete and locally validated.

The renderer version is `0.11.0-dev`. Every swapchain image owns a corresponding device-local depth target, and command recording selects that target using the acquired image index. The existing image fence therefore protects reuse of both the presentation image and its depth attachment.

The preserved rendering contract contains 96 vertices, 144 indices, 96 flat normals, device-local mesh buffers, directional lighting and two descriptor-backed frame-local scene uniform buffers.

Linux validation contains 19 passing tests. Repository history and the final synchronized `main` branch remain authoritative for the merged commit identity.
