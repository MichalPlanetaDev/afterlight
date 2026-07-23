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
                "Afterlight 0[.]13[.]0-dev [|] platform=dummy [|] window=1280x720"
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
                LABELS "integration;application;vulkan;presentation;shader;geometry;buffer;camera;normal;lighting;descriptor;uniform;depth-ownership;texture;material;sampler;texture-coordinate"
                PASS_REGULAR_EXPRESSION
                    "Afterlight 0[.]13[.]0-dev.*depth=[0-9]+.*depth-targets=[0-9]+.*depth-ownership=per-swapchain-image.*geometry=extruded-observatory-aperture.*vertices=96.*indices=144.*normals=96.*texture-coordinates=96.*texture-mapping=explicit-planar-radial.*mesh-memory=device-local.*lighting=directional.*uniforms=descriptor-set.*uniform-frames=2.*material=procedural-calibration.*texture-size=64x64.*texture-memory=device-local.*texture-binding=set1-image0-sampler1.*texture-checksum=ad3a091625158275.*validation=on"
                TIMEOUT 30
        )
    endif()
endif()
