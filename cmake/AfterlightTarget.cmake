function(afterlight_configure_target target)
    target_compile_features(${target} PRIVATE cxx_std_23)

    afterlight_enable_warnings(${target})
    afterlight_enable_sanitizers(${target})
endfunction()
