add_executable(
    afterlight_vulkan_device_selection_tests
    device_selection_test.cpp
)

target_link_libraries(
    afterlight_vulkan_device_selection_tests
    PRIVATE
        Afterlight::Vulkan
)

afterlight_configure_target(
    afterlight_vulkan_device_selection_tests
)

add_test(
    NAME afterlight.vulkan.device_selection
    COMMAND afterlight_vulkan_device_selection_tests
)

set_tests_properties(
    afterlight.vulkan.device_selection
    PROPERTIES
        LABELS "unit;graphics;vulkan"
)

add_executable(
    afterlight_vulkan_swapchain_policy_tests
    swapchain_policy_test.cpp
)

target_link_libraries(
    afterlight_vulkan_swapchain_policy_tests
    PRIVATE
        Afterlight::Vulkan
)

afterlight_configure_target(
    afterlight_vulkan_swapchain_policy_tests
)

add_test(
    NAME afterlight.vulkan.swapchain_policy
    COMMAND afterlight_vulkan_swapchain_policy_tests
)

set_tests_properties(
    afterlight.vulkan.swapchain_policy
    PROPERTIES
        LABELS "unit;graphics;vulkan;swapchain"
)

add_executable(
    afterlight_vulkan_barrier_translation_tests
    barrier_translation_test.cpp
)

target_link_libraries(
    afterlight_vulkan_barrier_translation_tests
    PRIVATE
        Afterlight::Vulkan
)

afterlight_configure_target(
    afterlight_vulkan_barrier_translation_tests
)

add_test(
    NAME afterlight.vulkan.barrier_translation
    COMMAND afterlight_vulkan_barrier_translation_tests
)

set_tests_properties(
    afterlight.vulkan.barrier_translation
    PROPERTIES
        LABELS "unit;graphics;vulkan;rhi;synchronization"
)

add_executable(
    afterlight_shader_binary_tests
    shader_binary_test.cpp
)

afterlight_configure_target(
    afterlight_shader_binary_tests
)

add_dependencies(
    afterlight_shader_binary_tests
    afterlight_shaders
)

add_test(
    NAME afterlight.shader.compiled_binaries
    COMMAND
        afterlight_shader_binary_tests
        "${AFTERLIGHT_VERTEX_SHADER_BINARY}"
        "${AFTERLIGHT_FRAGMENT_SHADER_BINARY}"
)

set_tests_properties(
    afterlight.shader.compiled_binaries
    PROPERTIES
        LABELS "unit;graphics;shader;hlsl;spirv"
)
