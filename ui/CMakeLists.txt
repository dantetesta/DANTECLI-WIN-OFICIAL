qt_add_library(dante_ui STATIC
    src/AppController.cpp
    include/dante/ui/AppController.hpp
)
add_library(dante::ui ALIAS dante_ui)

target_include_directories(dante_ui PUBLIC include)
target_compile_features(dante_ui PUBLIC cxx_std_23)

# AppController e um QObject exposto via context property "App" pelo app —
# nao precisa ser tipo QML, entao o ui/ NAO usa qt_add_qml_module (o modulo QML
# "Dante" vive no executavel, padrao canonico do Qt6 e sem dor de recurso estatico).
target_link_libraries(dante_ui
    PUBLIC
        dante::core
        dante::platform
        dante::persistence
        Qt6::Core
)

dante_target_warnings(dante_ui)
