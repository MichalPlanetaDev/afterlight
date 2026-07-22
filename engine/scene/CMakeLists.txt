find_package(glm CONFIG REQUIRED)

add_library(
    afterlight_scene
    STATIC
        src/camera.cpp
        src/mesh.cpp
)

add_library(Afterlight::Scene ALIAS afterlight_scene)

target_include_directories(
    afterlight_scene
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_link_libraries(
    afterlight_scene
    PRIVATE
        glm::glm
)

target_compile_definitions(
    afterlight_scene
    PRIVATE
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_FORCE_RADIANS
)

afterlight_configure_target(afterlight_scene)
