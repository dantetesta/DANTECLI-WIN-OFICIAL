add_library(dante_core STATIC
    src/Ids.cpp
    src/VtParser.cpp
    src/Screen.cpp
    src/Router.cpp
)
add_library(dante::core ALIAS dante_core)

target_include_directories(dante_core PUBLIC include)
target_compile_features(dante_core PUBLIC cxx_std_23)

dante_target_warnings(dante_core)

# Always build the proof so a broken core fails the compile; register with ctest
# only when the superbuild turned testing on (root: include(CTest)).
add_executable(dante_core_test tests/test_result.cpp)
target_link_libraries(dante_core_test PRIVATE dante::core)
dante_target_warnings(dante_core_test)

add_executable(dante_vt_test tests/test_vt.cpp)
target_link_libraries(dante_vt_test PRIVATE dante::core)
dante_target_warnings(dante_vt_test)

add_executable(dante_router_test tests/test_router.cpp)
target_link_libraries(dante_router_test PRIVATE dante::core)
dante_target_warnings(dante_router_test)

if(BUILD_TESTING)
    add_test(NAME dante_core_test COMMAND dante_core_test)
    add_test(NAME dante_vt_test COMMAND dante_vt_test)
    add_test(NAME dante_router_test COMMAND dante_router_test)
endif()
