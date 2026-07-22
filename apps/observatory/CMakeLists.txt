configure_file(
    shader_paths.hpp.in
    "${CMAKE_CURRENT_BINARY_DIR}/generated/afterlight/observatory/shader_paths.hpp"
    @ONLY
)

add_executable(
    afterlight_observatory
    main.cpp
)

set_target_properties(
    afterlight_observatory
    PROPERTIES
        OUTPUT_NAME afterlight
)

target_include_directories(
    afterlight_observatory
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

target_link_libraries(
    afterlight_observatory
    PRIVATE
        Afterlight::Core
        Afterlight::Platform
        Afterlight::Vulkan
)

add_dependencies(
    afterlight_observatory
    afterlight_shaders
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
                "Afterlight 0[.]5[.]0-dev [|] platform=dummy [|] window=1280x720"
            TIMEOUT 15
    )

    if(AFTERLIGHT_ENABLE_VULKAN_RUNTIME_TESTS)
        add_test(
            NAME afterlight.app.vulkan_geometry_smoke
            COMMAND afterlight_observatory --vulkan-smoke
        )

        set_tests_properties(
            afterlight.app.vulkan_geometry_smoke
            PROPERTIES
                LABELS "integration;application;vulkan;presentation;shader;geometry"
                PASS_REGULAR_EXPRESSION
                    "Afterlight 0[.]5[.]0-dev [|] backend=vulkan [|] device=.+ [|] presented=3 [|] extent=[0-9]+x[0-9]+ [|] images=[0-9]+ [|] format=[0-9]+ [|] geometry=triangle [|] validation=on"
                TIMEOUT 30
        )
    endif()
endif()
