# IPC Named Pipe: lib SEPARADA e Qt-free — o dante.exe (CLI) linka só isto + core, sem puxar Qt.
add_library(dante_ipc STATIC src/ipc/NamedPipe.cpp)
add_library(dante::ipc ALIAS dante_ipc)
target_include_directories(dante_ipc PUBLIC include)
target_compile_features(dante_ipc PUBLIC cxx_std_23)
target_link_libraries(dante_ipc PUBLIC dante::core)
if(WIN32)
    target_compile_definitions(dante_ipc PRIVATE _WIN32_WINNT=0x0A00 WIN32_LEAN_AND_MEAN NOMINMAX)
endif()
dante_target_warnings(dante_ipc)

add_library(dante_platform STATIC
    src/pty/ConPtyBackend.cpp
    src/PtyFactory.cpp
    src/memstat/MemStat.cpp
    src/secrets/DpapiSecrets.cpp
    src/SysStats.cpp
)
add_library(dante::platform ALIAS dante_platform)

target_include_directories(dante_platform PUBLIC include)
target_compile_features(dante_platform PUBLIC cxx_std_23)

# core: PUBLIC (headers expoem dante::IPtyBackend/Result). Qt: PRIVATE (so impl).
target_link_libraries(dante_platform
    PUBLIC dante::core dante::ipc
    PRIVATE Qt6::Core
)

# GetProcessMemoryInfo (gotcha #8) vive em Psapi no Windows.
if(WIN32)
    # Psapi: GetProcessMemoryInfo (gotcha #8). Crypt32: DPAPI (CryptProtectData) dos segredos.
    target_link_libraries(dante_platform PRIVATE Psapi Crypt32)
    # ConPTY exige Windows 10+ (_WIN32_WINNT). Os headers ja se autoprotegem, mas
    # fixamos no alvo pra consistencia entre TUs. WIN32_LEAN_AND_MEAN/NOMINMAX evitam ruido.
    target_compile_definitions(dante_platform PRIVATE
        _WIN32_WINNT=0x0A00 WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

dante_target_warnings(dante_platform)

# F1: teste de comportamento do ConPTY. No Windows o ctest spawna cmd.exe de verdade;
# fora do Windows compila como no-op. Roda no CI Windows.
add_executable(dante_conpty_test tests/test_conpty.cpp)
target_link_libraries(dante_conpty_test PRIVATE dante::platform)
target_compile_features(dante_conpty_test PRIVATE cxx_std_23)
dante_target_warnings(dante_conpty_test)
add_executable(dante_ipc_test tests/test_ipc.cpp)
target_link_libraries(dante_ipc_test PRIVATE dante::ipc)
target_compile_features(dante_ipc_test PRIVATE cxx_std_23)
dante_target_warnings(dante_ipc_test)

if(BUILD_TESTING)
    add_test(NAME dante_conpty_test COMMAND dante_conpty_test)
    add_test(NAME dante_ipc_test COMMAND dante_ipc_test)
endif()
