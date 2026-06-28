import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: win
    visible: true
    width: 1100
    height: 720
    title: App.appName + " — " + App.version
    color: "#0E0E11"

    Component.onCompleted: Term.start()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header
        Rectangle {
            Layout.fillWidth: true
            height: 46
            color: "#15151A"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 10

                Text {
                    text: App.appName
                    color: "#F5F5F7"
                    font.pixelSize: 16
                    font.bold: true
                }
                Text {
                    text: App.version
                    color: "#7A7A85"
                    font.pixelSize: 13
                }
                Item { Layout.fillWidth: true }
                Rectangle {
                    width: dot.implicitWidth + 18
                    height: 24
                    radius: 12
                    color: "#1E1E26"
                    Text {
                        id: dot
                        anchors.centerIn: parent
                        text: Term.running ? "● shell ativo" : "○ shell parado"
                        color: Term.running ? "#5BD675" : "#7A7A85"
                        font.pixelSize: 12
                    }
                }
            }
        }

        // Saída do terminal
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
                selectionColor: "#3A6EA5"
                font.family: "Consolas"
                font.pixelSize: 13
                leftPadding: 14
                topPadding: 12
                background: Rectangle { color: "#0E0E11" }
                onTextChanged: cursorPosition = length // auto-scroll p/ o fim
            }
        }

        // Entrada (modo linha): digite + Enter envia ao shell
        Rectangle {
            Layout.fillWidth: true
            height: 48
            color: "#15151A"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 8

                Text {
                    text: "›"
                    color: "#5BD675"
                    font.pixelSize: 18
                    font.bold: true
                }
                TextField {
                    id: input
                    Layout.fillWidth: true
                    focus: true
                    placeholderText: "comando + Enter  (ex.: dir, echo oi, ipconfig)  —  só funciona no Windows"
                    color: "#F5F5F7"
                    placeholderTextColor: "#6A6A75"
                    font.family: "Consolas"
                    font.pixelSize: 14
                    leftPadding: 10
                    background: Rectangle { color: "#1E1E26"; radius: 8 }
                    onAccepted: {
                        Term.send(text + "\r")
                        text = ""
                    }
                }
            }
        }
    }
}
