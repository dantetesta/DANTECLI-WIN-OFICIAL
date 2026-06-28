# Executavel da GUI = DanteCLI (nome do produto). O nome 'dante' fica reservado
# para o CLI helper (tools/dante, cliente Named Pipe).
# IMPORTANTE: o alvo NAO pode se chamar 'dante' porque o modulo QML URI "Dante"
# cria a pasta build/app/Dante/ — em filesystem case-insensitive (macOS e Windows)
# 'Dante/' e 'dante' colidem e o linker falha com EISDIR.
qt_add_executable(DanteCLI
    src/main.cpp
    src/Logging.cpp
)

# O executavel e o dono do modulo QML "Dante" (padrao canonico do Qt6).
# Mantem o modulo fora de lib estatica e evita o "No type named Main" causado
# pelo descarte de recursos QML no link de static libs.
qt_add_qml_module(DanteCLI
    URI Dante
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_compile_features(DanteCLI PRIVATE cxx_std_23)

target_link_libraries(DanteCLI
    PRIVATE
        dante::ui
        Qt6::Gui
        Qt6::Quick
)

# Janela sem console anexado no Windows.
if(WIN32)
    set_target_properties(DanteCLI PROPERTIES WIN32_EXECUTABLE ON)
endif()

dante_target_warnings(DanteCLI)
