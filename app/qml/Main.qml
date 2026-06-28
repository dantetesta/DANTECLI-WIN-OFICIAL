import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: window
    visible: true
    width: 1100
    height: 720
    title: App.appName
    color: "#0E0E11"

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 28

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: App.appName
            color: "#F5F5F7"
            font.pixelSize: 64
            font.bold: true
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "versão " + App.version
            color: "#8A8A93"
            font.pixelSize: 20
        }

        // Card que prova a fluidez: hover muda cor, pressed escala 0.97
        // (equivalente direto ao .animation do SwiftUI).
        Rectangle {
            id: card
            Layout.alignment: Qt.AlignHCenter
            width: 360
            height: 120
            radius: 12
            color: cardArea.containsMouse ? "#26262E" : "#1A1A20"
            scale: cardArea.pressed ? 0.97 : 1.0

            Behavior on color { ColorAnimation { duration: 150 } }
            Behavior on scale { NumberAnimation { duration: 120 } }

            Text {
                anchors.centerIn: parent
                text: "Fase 0 — fundação pronta"
                color: "#F5F5F7"
                font.pixelSize: 22
            }

            MouseArea {
                id: cardArea
                anchors.fill: parent
                hoverEnabled: true
            }
        }
    }
}
