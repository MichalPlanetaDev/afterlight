option(
    AFTERLIGHT_WARNINGS_AS_ERRORS
    "Treat warnings in project-owned code as errors"
    ON
)

option(
    AFTERLIGHT_ENABLE_SANITIZERS
    "Enable AddressSanitizer and UndefinedBehaviorSanitizer"
    OFF
)

option(
    AFTERLIGHT_ENABLE_CLANG_TIDY
    "Run clang-tidy during compilation"
    OFF
)

function(afterlight_configure_project_analysis)
    if(NOT AFTERLIGHT_ENABLE_CLANG_TIDY)
        return()
    endif()

    find_program(
        AFTERLIGHT_CLANG_TIDY_EXECUTABLE
        NAMES clang-tidy
        REQUIRED
    )

    set(
        CMAKE_CXX_CLANG_TIDY
        "${AFTERLIGHT_CLANG_TIDY_EXECUTABLE};--warnings-as-errors=*"
        PARENT_SCOPE
    )
endfunction()
