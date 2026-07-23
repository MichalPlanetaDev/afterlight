# Decisions

P11 owns one depth target per swapchain image.

The acquired image index selects both the color image and its depth attachment. The existing image-specific fence protects reuse of that attachment pair before command recording begins.

Every target uses the current swapchain extent and one validated depth format. Swapchain creation rejects empty or unrepresentable target counts and inconsistent formats.

This milestone does not introduce multisampling, depth sampling, depth pyramids, transient graph allocation or texture-material descriptors.
