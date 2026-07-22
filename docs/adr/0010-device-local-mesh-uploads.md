# ADR 0010: Device-local static mesh uploads

## Context

P09 moved scene-frame state into descriptor-backed frame-local uniform buffers, while the deterministic aperture vertex and index data remained directly writable by the CPU. That policy was sufficient for renderer bring-up but did not establish the transfer path required by later texture and compiled-asset uploads.

## Decision

Static aperture geometry is uploaded through host-visible staging buffers into device-local destination buffers. Staging allocations prefer host-coherent memory and use an explicit mapped-memory flush when coherent memory is unavailable.

A one-time graphics command buffer records the vertex and index copies. Synchronization2 buffer barriers make transfer writes visible to vertex-attribute and index reads. Fence completion occurs before staging allocations and the transient command pool are destroyed.

## Consequences

The final mesh buffers are not CPU mapped. Startup performs one bounded synchronous transfer, which is appropriate for the current static scene. Later asynchronous asset streaming requires a separate measured design for batching, queue ownership, cancellation and deferred lifetime management.
