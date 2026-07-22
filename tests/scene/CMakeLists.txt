add_executable(
    afterlight_scene_mesh_tests
    mesh_test.cpp
)

target_link_libraries(
    afterlight_scene_mesh_tests
    PRIVATE
        Afterlight::Scene
)

afterlight_configure_target(
    afterlight_scene_mesh_tests
)

add_test(
    NAME afterlight.scene.mesh
    COMMAND afterlight_scene_mesh_tests
)

set_tests_properties(
    afterlight.scene.mesh
    PROPERTIES
        LABELS "unit;scene;mesh;geometry"
)

add_executable(
    afterlight_scene_camera_tests
    camera_test.cpp
)

target_link_libraries(
    afterlight_scene_camera_tests
    PRIVATE
        Afterlight::Scene
)

afterlight_configure_target(
    afterlight_scene_camera_tests
)

add_test(
    NAME afterlight.scene.camera
    COMMAND afterlight_scene_camera_tests
)

set_tests_properties(
    afterlight.scene.camera
    PROPERTIES
        LABELS "unit;scene;camera;transform"
)

add_executable(
    afterlight_scene_lighting_contract_tests
    lighting_contract_test.cpp
)

target_link_libraries(
    afterlight_scene_lighting_contract_tests
    PRIVATE
        Afterlight::Scene
)

afterlight_configure_target(
    afterlight_scene_lighting_contract_tests
)

add_test(
    NAME afterlight.scene.lighting_contract
    COMMAND afterlight_scene_lighting_contract_tests
)

set_tests_properties(
    afterlight.scene.lighting_contract
    PROPERTIES
        LABELS "unit;scene;lighting;normal"
)
