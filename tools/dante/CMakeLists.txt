# dante.exe — cliente CLI Named Pipe. Qt-free (só dante::ipc + core). O nome 'dante' é reservado
# p/ este helper (o exe da GUI é DanteCLI — ver gotcha da colisão com o URI do módulo QML).
add_executable(dante main.cpp)
target_link_libraries(dante PRIVATE dante::ipc dante::core)
target_compile_features(dante PRIVATE cxx_std_23)
dante_target_warnings(dante)
