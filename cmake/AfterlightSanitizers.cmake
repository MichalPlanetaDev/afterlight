function(afterlight_enable_sanitizers target)
    if(NOT AFTERLIGHT_ENABLE_SANITIZERS)
        return()
    endif()

    if(MSVC)
        message(
            FATAL_ERROR
            "The current Milestone Zero sanitizer preset supports Clang and GCC."
        )
    endif()

    target_compile_options(
        ${target}
        PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
    )

    target_link_options(
        ${target}
        PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
    )
endfunction()
