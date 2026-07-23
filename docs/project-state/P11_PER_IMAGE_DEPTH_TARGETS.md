# P11 — Per-Swapchain-Image Depth Targets

P11 removes the shared writable depth attachment from the multi-frame renderer.

Each swapchain image now owns one device-local depth target. The acquired image index selects both attachments, and the image-specific fence protects reuse of the pair. Swapchain recreation destroys and rebuilds the complete target set.

Runtime smoke validation requires the depth-target count to equal the swapchain-image count while preserving all P10 geometry, upload, lighting and descriptor-uniform contracts.

P11_STATUS=PER_IMAGE_DEPTH_TARGETS_VALIDATED
