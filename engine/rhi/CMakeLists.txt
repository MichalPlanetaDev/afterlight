add_library(
    afterlight_rhi
    STATIC
        src/frame_scheduler.cpp
        src/resource.cpp
        src/resource_state_tracker.cpp
)

add_library(Afterlight::Rhi ALIAS afterlight_rhi)

target_include_directories(
    afterlight_rhi
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

afterlight_configure_target(afterlight_rhi)
