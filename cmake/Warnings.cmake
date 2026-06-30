# dante_target_warnings(<target>): strict warnings-as-errors per compiler.
function(dante_target_warnings target)
    if(MSVC)
        # /utf-8: fontes e literais em UTF-8 (projeto em português) — sem isso o MSVC lê
        # no codepage do sistema e quebra char constants tipo U'á' (C2015).
        # /external:* : headers de terceiros (Qt, stdlib via <...>) viram "external" em W0 —
        # sem isso o /WX transforma um warning DENTRO de header do Qt (ex.: C4702 unreachable
        # em qjsprimitivevalue.h, puxado por QVariant/QML) em erro de build. templates- faz o
        # warning de instanciacao ser atribuido ao header externo, nao ao nosso TU.
        target_compile_options(${target} PRIVATE
            /W4 /permissive- /WX /utf-8
            /external:anglebrackets /external:templates- /external:W0)
    else()
        # -Wno-unused-parameter so empty F0 stubs don't trip -Werror.
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Werror -Wno-unused-parameter)
    endif()
endfunction()
