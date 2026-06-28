# dante_target_warnings(<target>): strict warnings-as-errors per compiler.
function(dante_target_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /WX)
    else()
        # -Wno-unused-parameter so empty F0 stubs don't trip -Werror.
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Werror -Wno-unused-parameter)
    endif()
endfunction()
