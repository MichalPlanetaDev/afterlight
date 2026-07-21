add_executable(
    afterlight_rhi_resource_registry_tests
    resource_registry_test.cpp
)

target_link_libraries(
    afterlight_rhi_resource_registry_tests
    PRIVATE
        Afterlight::Rhi
)

afterlight_configure_target(
    afterlight_rhi_resource_registry_tests
)

add_test(
    NAME afterlight.rhi.resource_registry
    COMMAND afterlight_rhi_resource_registry_tests
)

set_tests_properties(
    afterlight.rhi.resource_registry
    PROPERTIES
        LABELS "unit;rhi;resources"
)

add_executable(
    afterlight_rhi_frame_scheduler_tests
    frame_scheduler_test.cpp
)

target_link_libraries(
    afterlight_rhi_frame_scheduler_tests
    PRIVATE
        Afterlight::Rhi
)

afterlight_configure_target(
    afterlight_rhi_frame_scheduler_tests
)

add_test(
    NAME afterlight.rhi.frame_scheduler
    COMMAND afterlight_rhi_frame_scheduler_tests
)

set_tests_properties(
    afterlight.rhi.frame_scheduler
    PROPERTIES
        LABELS "unit;rhi;frames;synchronization"
)
