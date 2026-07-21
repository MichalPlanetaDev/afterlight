add_executable(
    afterlight_core_tests
    build_info_test.cpp
)

target_link_libraries(
    afterlight_core_tests
    PRIVATE
        Afterlight::Core
)

afterlight_configure_target(afterlight_core_tests)

add_test(
    NAME afterlight.core.build_info
    COMMAND afterlight_core_tests
)

set_tests_properties(
    afterlight.core.build_info
    PROPERTIES
        LABELS "unit;core"
)

add_executable(
    afterlight_lifecycle_tests
    application_lifecycle_test.cpp
)

target_link_libraries(
    afterlight_lifecycle_tests
    PRIVATE
        Afterlight::Core
)

afterlight_configure_target(afterlight_lifecycle_tests)

add_test(
    NAME afterlight.core.application_lifecycle
    COMMAND afterlight_lifecycle_tests
)

set_tests_properties(
    afterlight.core.application_lifecycle
    PROPERTIES
        LABELS "unit;core;lifecycle"
)

add_executable(
    afterlight_window_config_tests
    window_config_test.cpp
)

target_link_libraries(
    afterlight_window_config_tests
    PRIVATE
        Afterlight::Platform
)

afterlight_configure_target(
    afterlight_window_config_tests
)

add_test(
    NAME afterlight.platform.window_config
    COMMAND afterlight_window_config_tests
)

set_tests_properties(
    afterlight.platform.window_config
    PROPERTIES
        LABELS "unit;platform"
)
