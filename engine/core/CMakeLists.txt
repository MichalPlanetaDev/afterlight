add_library(
    afterlight_core
    STATIC
        src/application_lifecycle.cpp
        src/build_info.cpp
)

add_library(Afterlight::Core ALIAS afterlight_core)

target_include_directories(
    afterlight_core
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

afterlight_configure_target(afterlight_core)
