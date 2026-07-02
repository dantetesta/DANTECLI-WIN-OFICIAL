add_library(dante_persistence STATIC
    src/Database.cpp
    src/MigrationRunner.cpp
    src/KvStore.cpp
)
add_library(dante::persistence ALIAS dante_persistence)

target_include_directories(dante_persistence PUBLIC include)
target_compile_features(dante_persistence PUBLIC cxx_std_23)

# Qt6::Core is PUBLIC: QString appears in the public Database.hpp API.
# Qt6::Sql is PRIVATE: it only lives in the .cpp files (connections fetched by name).
target_link_libraries(dante_persistence
    PUBLIC
        dante::core
        Qt6::Core
    PRIVATE
        Qt6::Sql
)

dante_target_warnings(dante_persistence)

# Round-trip de persistência (migração + KvStore). Roda no mac e no CI.
add_executable(dante_kv_test tests/test_kv.cpp)
target_link_libraries(dante_kv_test PRIVATE dante::persistence Qt6::Core)
target_compile_features(dante_kv_test PRIVATE cxx_std_23)
dante_target_warnings(dante_kv_test)
if(BUILD_TESTING)
    add_test(NAME dante_kv_test COMMAND dante_kv_test)
endif()
