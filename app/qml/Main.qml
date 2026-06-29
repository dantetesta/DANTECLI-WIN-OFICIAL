import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: win
    visible: true
    width: 1320
    height: 820
    title: "DANTE CLI"
    color: "#0E0E11"

    // ---- paleta (extraída do app Swift) ----
    readonly property color cBg:      "#0E0E11"
    readonly property color cPanel:   "#161619"
    readonly property color cPanel2:  "#1E1E24"
    readonly property color cTermBg:  "#0A0A0C"
    readonly property color cAccent:  "#2D6BF0"
    readonly property color cText:    "#E8E8EC"
    readonly property color cDim:     "#7E7E88"
    readonly property color cBorder:  "#262630"

    Component.onCompleted: Term.start()

    // ===== componentes reutilizáveis =====
    component Pill: Rectangle {
        property alias label: t.text
        property color fg: "#B8B8C0"
        radius: 6
        color: "#23232A"
        implicitWidth: t.implicitWidth + 16
        implicitHeight: 22
        Text { id: t; anchors.centerIn: parent; color: parent.fg; font.pixelSize: 11; font.bold: true }
    }

    component TBtn: Rectangle {
        id: tb
        property string glyph: ""
        property string label: ""
        property color tint: win.cText
        signal clicked()
        implicitWidth: r.implicitWidth + 18
        implicitHeight: 30
        radius: 7
        color: ma.containsMouse ? "#26262E" : "transparent"
        Behavior on color { ColorAnimation { duration: 120 } }
        Row {
            id: r
            anchors.centerIn: parent
            spacing: 6
            Text { text: tb.glyph; color: tb.tint; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
            Text { visible: tb.label.length > 0; text: tb.label; color: tb.tint; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter }
        }
        MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; onClicked: tb.clicked() }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ============ SIDEBAR ESQUERDA ============
        Rectangle {
            Layout.preferredWidth: 258
            Layout.fillHeight: true
            color: cPanel

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // tabs de modo: Favoritos / Pastas / Snippets / Chaves
                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: 10
                    spacing: 6
                    Repeater {
                        model: [
                            { ic: "★",  tx: "Favoritos" },
                            { ic: "🗂", tx: "Pastas" },
                            { ic: "❮❯", tx: "Snippets" },
                            { ic: "🔑", tx: "Chaves" }
                        ]
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            implicitHeight: 52
                            radius: 9
                            color: index === 1 ? cAccent : (mma.containsMouse ? "#23232A" : "transparent")
                            Behavior on color { ColorAnimation { duration: 120 } }
                            Column {
                                anchors.centerIn: parent
                                spacing: 3
                                Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.ic; font.pixelSize: 16; color: index === 1 ? "white" : cDim }
                                Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.tx; font.pixelSize: 10; color: index === 1 ? "white" : cDim }
                            }
                            MouseArea { id: mma; anchors.fill: parent; hoverEnabled: true }
                        }
                    }
                }

                // cabeçalho da pasta
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 14
                    Layout.topMargin: 4
                    Text { text: "dantetesta"; color: cText; font.pixelSize: 13; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Text { text: "•••"; color: cDim; font.pixelSize: 12 }
                    Text { text: "👁"; color: cDim; font.pixelSize: 12 }
                    Text { text: "⟳"; color: cDim; font.pixelSize: 13 }
                }
                Text { text: "~"; color: cDim; font.pixelSize: 12; Layout.leftMargin: 16; Layout.topMargin: 2 }

                // árvore de arquivos
                ListView {
                    id: tree
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.topMargin: 6
                    clip: true
                    model: [
                        "_img","Applications","backups","claude-cli","Creative Cloud","Desktop",
                        "Documents","Downloads","evolution-api","generated_imgs","go","Local",
                        "Movies","Music","node_modules","Pictures","Projetos","Public","skills",
                        "Studio","tmp","vscode_sync_tool","§example-nanobanana.md","§GEMINI.md"
                    ]
                    delegate: Rectangle {
                        required property var modelData
                        width: tree.width
                        height: 26
                        property bool isFile: modelData.charAt(0) === "§"
                        property string nm: isFile ? modelData.substring(1) : modelData
                        color: rma.containsMouse ? "#20202A" : "transparent"
                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            spacing: 6
                            Text { text: isFile ? "  " : "▸"; color: cDim; font.pixelSize: 10; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: isFile ? "📄" : "📁"; font.pixelSize: 13; color: isFile ? cDim : "#5AA0F0"; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: nm; color: isFile ? cDim : cText; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter }
                        }
                        MouseArea { id: rma; anchors.fill: parent; hoverEnabled: true }
                    }
                }
            }
        }

        Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: cBorder }

        // ============ COLUNA PRINCIPAL ============
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ---- TOOLBAR ----
            Rectangle {
                Layout.fillWidth: true
                height: 52
                color: cPanel
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 12
                    spacing: 8

                    Text { text: "Workspace 1"; color: cText; font.pixelSize: 16; font.bold: true }
                    Item { Layout.fillWidth: true }

                    // medidores CPU/RAM
                    Pill { label: "18% CPU"; fg: "#E0A23B" }
                    Pill { label: "100M"; fg: "#9AA0AA" }

                    TBtn { glyph: "➕" }
                    TBtn { glyph: "🌐" }
                    TBtn { glyph: "🎭" }
                    TBtn { glyph: "▶" }
                    TBtn { glyph: "🎤" }
                    TBtn { glyph: "✕"; label: "Limpar"; tint: "#E86A78" }
                    TBtn { glyph: "▦"; label: "Split Layout" }
                    TBtn { glyph: "⎋"; label: "Sair"; tint: "#FF5A6E" }
                    Rectangle {
                        implicitWidth: cb.implicitWidth + 22; implicitHeight: 32; radius: 8
                        color: "#23232E"; border.color: "#3A3550"; border.width: 1
                        Row { id: cb; anchors.centerIn: parent; spacing: 6
                            Text { text: "✦"; color: "#B98BFF"; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: "Claude"; color: cText; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter }
                        }
                    }
                }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: cBorder }

            // ---- TAB BAR ----
            Rectangle {
                Layout.fillWidth: true
                height: 40
                color: cBg
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    spacing: 6
                    Rectangle {
                        implicitWidth: tabRow.implicitWidth + 26
                        implicitHeight: 30
                        radius: 8
                        color: "#1B2740"
                        border.color: cAccent; border.width: 1
                        Row { id: tabRow; anchors.centerIn: parent; spacing: 7
                            Text { text: "📁"; font.pixelSize: 12; color: "#5AA0F0"; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: "DS Fácil"; color: cText; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: "✕"; color: cDim; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
                        }
                    }
                    TBtn { glyph: "➕"; tint: cDim }
                    Item { Layout.fillWidth: true }
                }
            }

            // ---- PAINEL (terminal) ----
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 10
                radius: 10
                color: cTermBg
                border.color: cAccent
                border.width: 1.5
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // header azul gradiente
                    Rectangle {
                        Layout.fillWidth: true
                        height: 38
                        radius: 9
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: "#1E5FD0" }
                            GradientStop { position: 1.0; color: "#2E7BF0" }
                        }
                        // canto inferior reto (cobre o radius de baixo)
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 10; color: "#256BE0" }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 10
                            spacing: 8
                            Text { text: "📁"; font.pixelSize: 13; color: "white" }
                            Text { text: "DS Fácil"; color: "white"; font.pixelSize: 13; font.bold: true }
                            Text { text: "~/Desktop/Fable5/DSFACIL/ds-facil"; color: "#D6E4FF"; font.pixelSize: 12 }
                            Item { Layout.fillWidth: true }
                            Pill { label: "652M"; fg: "#D6E4FF"; color: "#1B4FB0" }
                            Pill { label: "518%"; fg: "#FFCC66"; color: "#1B4FB0" }
                            Rectangle {
                                radius: 6; implicitWidth: av.implicitWidth + 16; implicitHeight: 22; color: "#15C2D6"
                                Text { id: av; anchors.centerIn: parent; text: Term.running ? "ATIVO" : "—"; color: "#062B30"; font.pixelSize: 11; font.bold: true }
                            }
                            Text { text: "⚙"; color: "#D6E4FF"; font.pixelSize: 13 }
                            Text { text: "⤢"; color: "#D6E4FF"; font.pixelSize: 13 }
                        }
                    }

                    // saída do terminal
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        TextArea {
                            id: term
                            readOnly: true
                            wrapMode: TextArea.WrapAnywhere
                            text: Term.output
                            color: "#D6D6DA"
                            font.family: "Consolas"
                            font.pixelSize: 13
                            leftPadding: 14
                            topPadding: 10
                            background: Rectangle { color: cTermBg }
                            onTextChanged: cursorPosition = length
                        }
                    }

                    // linha de prompt
                    Rectangle {
                        Layout.fillWidth: true
                        height: 42
                        color: "#0C0C0F"
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 12
                            spacing: 8
                            Text { text: "›"; color: "#5BD675"; font.pixelSize: 18; font.bold: true }
                            TextField {
                                id: input
                                Layout.fillWidth: true
                                focus: true
                                placeholderText: "digite um comando e Enter (ex.: dir, echo oi, ipconfig)"
                                color: cText
                                placeholderTextColor: "#5A5A64"
                                font.family: "Consolas"
                                font.pixelSize: 14
                                background: Rectangle { color: "transparent" }
                                onAccepted: { Term.send(text + "\r"); text = "" }
                            }
                            Text { text: "auto mode on"; color: cDim; font.pixelSize: 11 }
                        }
                    }
                }
            }
        }
    }
}
