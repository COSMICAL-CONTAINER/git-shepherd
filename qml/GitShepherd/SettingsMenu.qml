import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Popup {
    id: root

    width: 240
    padding: 14
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: Theme.surface
        radius: 10
        border.width: 1
        border.color: Theme.border
    }

    contentItem: ColumnLayout {
        spacing: 10

        Text {
            text: qsTr("设置")
            color: Theme.text
            font.pixelSize: 14
            font.bold: true
        }

        Text {
            text: qsTr("外观")
            color: Theme.dim
            font.pixelSize: 12
        }

        Row {
            spacing: 6
            Layout.fillWidth: true

            Repeater {
                model: [ qsTr("深色"), qsTr("浅色") ]

                delegate: Rectangle {
                    id: seg
                    required property string modelData
                    required property int index
                    readonly property bool selected: AppSettings.darkTheme === (index === 0)

                    width: 96
                    height: 32
                    radius: 8
                    color: selected ? Theme.accent : Theme.surfaceAlt
                    border.width: selected ? 0 : 1
                    border.color: Theme.border

                    Text {
                        anchors.centerIn: parent
                        text: seg.modelData
                        color: seg.selected ? Theme.accentText : Theme.dim
                        font.pixelSize: 13
                        font.bold: seg.selected
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: AppSettings.darkTheme = seg.index === 0
                    }
                }
            }
        }
    }
}
