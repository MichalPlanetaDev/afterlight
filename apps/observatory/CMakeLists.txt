add_executable(afterlight_observatory main.cpp)

set_target_properties(
    afterlight_observatory
    PROPERTIES
        OUTPUT_NAME afterlight
)

target_link_libraries(
    afterlight_observatory
    PRIVATE
        Afterlight::Core
        Afterlight::Platform
)

afterlight_configure_target(afterlight_observatory)

if(BUILD_TESTING)
    add_test(
        NAME afterlight.app.platform_smoke
        COMMAND afterlight_observatory --smoke
    )

    set_tests_properties(
        afterlight.app.platform_smoke
        PROPERTIES
            LABELS "integration;application;platform"
            PASS_REGULAR_EXPRESSION
                "Afterlight 0[.]1[.]0-dev [|] platform=dummy [|] window=1280x720"
            TIMEOUT 15
    )
endif()
