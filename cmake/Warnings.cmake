# dante_target_warnings(<target>): strict warnings-as-errors per compiler.
function(dante_target_warnings target)
    if(MSVC)
        # /utf-8: fontes e literais em UTF-8 (projeto em português) — sem isso o MSVC lê
        # no codepage do sistema e quebra char constants tipo U'á' (C2015).
        target_compile_options(${target} PRIVATE /W4 /permissive- /WX /utf-8)
    else()
        # -Wno-unused-parameter so empty F0 stubs don't trip -Werror.
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Werror -Wno-unused-parameter)
    endif()
endfunction()
