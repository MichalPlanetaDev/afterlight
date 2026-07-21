find_package(SDL3 CONFIG REQUIRED)

add_library(
    afterlight_platform
    STATIC
        src/platform.cpp
)

add_library(Afterlight::Platform ALIAS afterlight_platform)

target_include_directories(
    afterlight_platform
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_link_libraries(
    afterlight_platform
    PRIVATE
        SDL3::SDL3
)

afterlight_configure_target(afterlight_platform)
