add_executable(
    afterlight_platform_smoke_tests
    platform_smoke_test.cpp
)

target_link_libraries(
    afterlight_platform_smoke_tests
    PRIVATE
        Afterlight::Platform
)

afterlight_configure_target(
    afterlight_platform_smoke_tests
)

add_test(
    NAME afterlight.platform.sdl_smoke
    COMMAND afterlight_platform_smoke_tests
)

set_tests_properties(
    afterlight.platform.sdl_smoke
    PROPERTIES
        LABELS "integration;platform;sdl"
        TIMEOUT 15
)
