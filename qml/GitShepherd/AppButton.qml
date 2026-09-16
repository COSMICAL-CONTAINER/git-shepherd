import QtQuick
import QtQuick.Controls.Basic

Button {
    id: root

    property string kind: "accent" // "accent" | "outline" | "ghost"

    readonly property bool isAccent: kind === "accent"
    readonly property bool isOutline: kind === "outline"

    implicitHeight: 34
    padding: 16
    spacing: 6
    hoverEnabled: true
    readonly property real enabledOpacity: enabled ? 1.0 : 0.45

    font.pixelSize: 13

    contentItem: Text {
        text: root.text
        font: root.font
        opacity: root.enabledOpacity
        color: root.isAccent ? Theme.accentText
             : root.hovered ? Theme.text
             : Theme.dim
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 8
        opacity: root.enabledOpacity
        color: root.isAccent
               ? (root.hovered ? Theme.accentHover : Theme.accent)
               : (root.hovered ? Theme.surfaceAlt : "transparent")
        border.width: root.isOutline ? 1 : 0
        border.color: Theme.border
    }
}
