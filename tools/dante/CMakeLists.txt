# dante.exe — cliente CLI Named Pipe. Qt-free (só dante::ipc + core). O nome 'dante' é reservado
# p/ este helper (o exe da GUI é DanteCLI — ver gotcha da colisão com o URI do módulo QML).
add_executable(dante main.cpp)
target_link_libraries(dante PRIVATE dante::ipc dante::core)
target_compile_features(dante PRIVATE cxx_std_23)
# std::getenv é C4996 ("inseguro") no MSVC e o /WX quebra. É a forma portátil de ler env var;
# suprimimos a deprecação SÓ neste alvo (o app usa qgetenv, não precisa disto).
if(WIN32)
    target_compile_definitions(dante PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
dante_target_warnings(dante)
