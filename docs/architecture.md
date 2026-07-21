# Architecture

Afterlight separates process policy, operating-system integration and
graphics implementation.

```text
observatory application
     |           |
     v           v
core runtime   platform
                 |
                SDL3
                 |
            native window
```

`afterlight_core` owns lifecycle state and build identity.
`afterlight_platform` owns SDL initialization, native-window lifetime
and event translation. SDL types remain inside the platform
implementation; the application consumes project-owned configuration,
size and event values.

The graphics layer is deliberately absent from this milestone. The next
boundary adds Vulkan instance creation and asks the platform module to
create a presentation surface without exposing window-system details to
renderer code.

The eventual rendering stack remains:

```text
application
    |
platform
    |
   RHI
 /     \
Vulkan  Direct3D 12
    |
render graph
    |
frame renderer
```

Vulkan is the reference implementation. Direct3D 12 is added against
proven resource and synchronization contracts rather than developed as
a parallel speculative abstraction.
