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
