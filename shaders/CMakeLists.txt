find_program(
    DIRECTX_DXC_TOOL
    NAMES
        dxc
        dxc.exe
    PATHS
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/directx-dxc"
    NO_DEFAULT_PATH
    REQUIRED
)

set(
    AFTERLIGHT_SHADER_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/generated/shaders"
    CACHE INTERNAL
    "Afterlight compiled shader directory"
)

set(
    AFTERLIGHT_VERTEX_SHADER_BINARY
    "${AFTERLIGHT_SHADER_OUTPUT_DIRECTORY}/mesh.vert.spv"
    CACHE INTERNAL
    "Afterlight mesh vertex shader"
)

set(
    AFTERLIGHT_FRAGMENT_SHADER_BINARY
    "${AFTERLIGHT_SHADER_OUTPUT_DIRECTORY}/mesh.frag.spv"
    CACHE INTERNAL
    "Afterlight mesh fragment shader"
)

set(
    AFTERLIGHT_MESH_SHADER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/mesh.hlsl"
)

add_custom_command(
    OUTPUT
        "${AFTERLIGHT_VERTEX_SHADER_BINARY}"
    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${AFTERLIGHT_SHADER_OUTPUT_DIRECTORY}"
    COMMAND
        "${DIRECTX_DXC_TOOL}"
        -spirv
        -fspv-target-env=vulkan1.3
        -HV 2021
        -Ges
        -T vs_6_0
        -E vs_main
        -Fo "${AFTERLIGHT_VERTEX_SHADER_BINARY}"
        "${AFTERLIGHT_MESH_SHADER_SOURCE}"
    DEPENDS
        "${AFTERLIGHT_MESH_SHADER_SOURCE}"
        "${DIRECTX_DXC_TOOL}"
    COMMENT
        "Compiling mesh vertex shader with DXC"
    VERBATIM
)

add_custom_command(
    OUTPUT
        "${AFTERLIGHT_FRAGMENT_SHADER_BINARY}"
    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${AFTERLIGHT_SHADER_OUTPUT_DIRECTORY}"
    COMMAND
        "${DIRECTX_DXC_TOOL}"
        -spirv
        -fspv-target-env=vulkan1.3
        -HV 2021
        -Ges
        -T ps_6_0
        -E ps_main
        -Fo "${AFTERLIGHT_FRAGMENT_SHADER_BINARY}"
        "${AFTERLIGHT_MESH_SHADER_SOURCE}"
    DEPENDS
        "${AFTERLIGHT_MESH_SHADER_SOURCE}"
        "${DIRECTX_DXC_TOOL}"
    COMMENT
        "Compiling mesh fragment shader with DXC"
    VERBATIM
)

add_custom_target(
    afterlight_shaders
    DEPENDS
        "${AFTERLIGHT_VERTEX_SHADER_BINARY}"
        "${AFTERLIGHT_FRAGMENT_SHADER_BINARY}"
)
