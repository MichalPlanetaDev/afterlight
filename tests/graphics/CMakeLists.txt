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

add_executable(
    afterlight_vulkan_depth_format_tests
    depth_format_test.cpp
)

target_link_libraries(
    afterlight_vulkan_depth_format_tests
    PRIVATE
        Afterlight::Vulkan
)

afterlight_configure_target(
    afterlight_vulkan_depth_format_tests
)

add_test(
    NAME afterlight.vulkan.depth_format
    COMMAND afterlight_vulkan_depth_format_tests
)

set_tests_properties(
    afterlight.vulkan.depth_format
    PROPERTIES
        LABELS "unit;graphics;vulkan;depth"
)

add_executable(
    afterlight_vulkan_scene_uniform_tests
    scene_uniforms_test.cpp
)

target_link_libraries(
    afterlight_vulkan_scene_uniform_tests
    PRIVATE
        Afterlight::Vulkan
)

afterlight_configure_target(
    afterlight_vulkan_scene_uniform_tests
)

add_test(
    NAME afterlight.vulkan.scene_uniforms
    COMMAND afterlight_vulkan_scene_uniform_tests
)

set_tests_properties(
    afterlight.vulkan.scene_uniforms
    PROPERTIES
        LABELS "unit;graphics;vulkan;descriptor;uniform"
)

add_executable(
    afterlight_vulkan_mesh_upload_tests
    mesh_upload_test.cpp
)

target_link_libraries(
    afterlight_vulkan_mesh_upload_tests
    PRIVATE
        Afterlight::Vulkan
)

afterlight_configure_target(
    afterlight_vulkan_mesh_upload_tests
)

add_test(
    NAME afterlight.vulkan.mesh_upload
    COMMAND afterlight_vulkan_mesh_upload_tests
)

set_tests_properties(
    afterlight.vulkan.mesh_upload
    PROPERTIES
        LABELS "unit;graphics;vulkan;upload;device-local"
)

add_executable(
    afterlight_vulkan_depth_target_policy_tests
    depth_target_policy_test.cpp
)

target_link_libraries(
    afterlight_vulkan_depth_target_policy_tests
    PRIVATE
        Afterlight::Vulkan
)

afterlight_configure_target(
    afterlight_vulkan_depth_target_policy_tests
)

add_test(
    NAME afterlight.vulkan.depth_target_policy
    COMMAND afterlight_vulkan_depth_target_policy_tests
)

set_tests_properties(
    afterlight.vulkan.depth_target_policy
    PROPERTIES
        LABELS "unit;graphics;vulkan;depth;swapchain;ownership"
)

add_executable(
    afterlight_vulkan_material_texture_tests
    material_texture_test.cpp
)

target_link_libraries(
    afterlight_vulkan_material_texture_tests
    PRIVATE
        Afterlight::Vulkan
)

afterlight_configure_target(
    afterlight_vulkan_material_texture_tests
)

add_test(
    NAME afterlight.vulkan.material_texture
    COMMAND afterlight_vulkan_material_texture_tests
)

set_tests_properties(
    afterlight.vulkan.material_texture
    PROPERTIES
        LABELS "unit;graphics;vulkan;texture;material;upload;descriptor"
)

add_executable(
    afterlight_vulkan_material_parameter_tests
    material_parameters_test.cpp
)

target_link_libraries(
    afterlight_vulkan_material_parameter_tests
    PRIVATE
        Afterlight::Vulkan
)

afterlight_configure_target(
    afterlight_vulkan_material_parameter_tests
)

add_test(
    NAME afterlight.vulkan.material_parameters
    COMMAND afterlight_vulkan_material_parameter_tests
)

set_tests_properties(
    afterlight.vulkan.material_parameters
    PROPERTIES
        LABELS "unit;graphics;vulkan;material;descriptor;uniform-buffer;persistent"
)
