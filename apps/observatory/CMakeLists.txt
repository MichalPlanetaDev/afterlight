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
)

afterlight_configure_target(afterlight_observatory)

if(BUILD_TESTING)
    add_test(
        NAME afterlight.app.foundation
        COMMAND afterlight_observatory
    )

    set_tests_properties(
        afterlight.app.foundation
        PROPERTIES
            LABELS "integration;application"
            PASS_REGULAR_EXPRESSION
                "Afterlight 0[.]0[.]0-dev [|] Milestone Zero [|] local"
    )
endif()
