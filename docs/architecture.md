# Architecture

Afterlight separates application policy from rendering implementation.
The executable owns composition and process-level decisions. Reusable
runtime behavior belongs to project libraries and does not depend on a
concrete window system or graphics API.

## Implemented boundary

```text
afterlight_observatory
          |
          v
   afterlight_core
     |         |
     v         v
build identity lifecycle
```

`afterlight_core` currently owns immutable build information and the
application lifecycle state machine. Lifecycle transitions are explicit,
invalid transitions return structured errors and normal shutdown requires
a stop request before teardown.

## Renderer boundary

The next architectural layer introduces a platform module and a
project-owned rendering hardware interface:

```text
observatory application
          |
     platform layer
          |
         RHI
      /       \
   Vulkan   Direct3D 12
          |
     render graph
          |
  renderer and debug views
```

Vulkan is implemented first. Direct3D 12 is added against the same
resource, synchronization and command-submission contracts only after the
Vulkan path is validated. Backend-specific behavior remains visible where
the APIs differ; the abstraction is not intended to erase meaningful
graphics concepts.

The renderer will use HLSL compiled through DXC to SPIR-V and DXIL. The
planned frame architecture combines deferred opaque rendering with
forward paths for transparency and specialist materials. Engineering
views expose real frame resources such as depth, normals, material data,
motion vectors, lighting, volumetrics and temporal history.

Generated geometry, materials, textures, animation and audio are treated
as deterministic build products. Their authored sources remain code and
versioned parameters rather than manually edited binary assets.
