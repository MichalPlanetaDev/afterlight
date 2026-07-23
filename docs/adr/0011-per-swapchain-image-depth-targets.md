# ADR 0011: Per-swapchain-image depth targets

## Context

The renderer supports two frames in flight and tracks completion independently for each acquired swapchain image. Before P11, every frame recorded depth reads and writes against one shared depth image. Different swapchain images could therefore reference the same writable depth allocation while their submissions were independently in flight.

Passing smoke tests did not prove that this shared attachment lifetime was valid on every Vulkan implementation. Depth ownership needed to align with the swapchain image whose fence already governs safe reuse.

## Decision

The swapchain owns one `DepthTarget` for every swapchain image. Command recording selects the depth target using the acquired image index. The existing image fence therefore protects both the presentation image and its corresponding depth attachment before that pair is reused.

All depth targets use the same extent and validated depth format. Swapchain creation rejects an empty image set, an unrepresentable target count, null ownership or inconsistent formats. Swapchain destruction clears the complete depth-target collection before image views and the swapchain are destroyed.

## Consequences

Depth memory consumption scales with swapchain image count. In exchange, attachment ownership and synchronization are explicit, resize recreation remains deterministic and independently in-flight frames no longer share one writable depth allocation.

This decision does not introduce multisampling, depth sampling, depth pyramids or transient render-graph allocation.
