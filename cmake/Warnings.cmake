# dante_target_warnings(<target>): strict warnings-as-errors per compiler.
function(dante_target_warnings target)
    if(MSVC)
        # /utf-8: fontes e literais em UTF-8 (projeto em português) — sem isso o MSVC lê
        # no codepage do sistema e quebra char constants tipo U'á' (C2015).
        # /external:* : headers de terceiros (Qt, stdlib via <...>) viram "external" em W0,
        # blindando warnings de FRONTEND dentro de headers do Qt contra o /WX.
        # /wd4702 : "unreachable code" e warning de BACKEND/otimizador — o /external NAO o
        # suprime (provem da codegen, sem rastrear o header de origem). Dispara dentro do
        # qjsprimitivevalue.h do Qt (puxado por QVariant/QML), nao no nosso codigo. C4702 e
        # notoriamente ruidoso (return apos throw etc.), entao desligamos de vez.
        target_compile_options(${target} PRIVATE
            /W4 /permissive- /WX /utf-8 /wd4702
            /external:anglebrackets /external:templates- /external:W0)
    else()
        # -Wno-unused-parameter so empty F0 stubs don't trip -Werror.
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Werror -Wno-unused-parameter)
    endif()
endfunction()
