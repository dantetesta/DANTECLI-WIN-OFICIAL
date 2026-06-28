add_library(dante_platform STATIC
    src/pty/ConPtyBackend.cpp
    src/memstat/MemStat.cpp
    src/secrets/DpapiSecrets.cpp
)
add_library(dante::platform ALIAS dante_platform)

target_include_directories(dante_platform PUBLIC include)
target_compile_features(dante_platform PUBLIC cxx_std_23)

# core: PUBLIC (headers expoem dante::IPtyBackend/Result). Qt: PRIVATE (so impl).
target_link_libraries(dante_platform
    PUBLIC dante::core
    PRIVATE Qt6::Core
)

# GetProcessMemoryInfo (gotcha #8) vive em Psapi no Windows.
if(WIN32)
    target_link_libraries(dante_platform PRIVATE Psapi)
endif()

dante_target_warnings(dante_platform)
