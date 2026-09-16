import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: root

    required property string state
    property int behind: 0
    property int ahead: 0

    readonly property var info: ({
        "idle":            { label: qsTr("待检测"), color: Theme.dim },
        "checking":        { label: qsTr("检测中"), color: Theme.dim },
        "updating":        { label: qsTr("更新中"), color: Theme.accent },
        "switching":       { label: qsTr("切换中"), color: Theme.accent },
        "upToDate":        { label: qsTr("最新"),   color: Theme.green },
        "updateAvailable": { label: qsTr("可更新"), color: Theme.amber },
        "dirty":           { label: qsTr("本地改动"), color: Theme.orange },
        "diverged":        { label: qsTr("分叉"),   color: Theme.purple },
        "noUpstream":      { label: qsTr("无上游"), color: Theme.dim },
        "error":           { label: qsTr("错误"),   color: Theme.red },
        "updated":         { label: qsTr("已更新"), color: Theme.green }
    })
    readonly property var current: info[state] !== undefined ? info[state] : info["idle"]
    readonly property bool spinning: state === "checking" || state === "updating"

    function labelText(): string {
        if (state === "diverged")
            return behind > 0 ? qsTr("已分叉") : qsTr("未推送");
        return current.label;
    }

    radius: height / 2
    implicitHeight: 22
    implicitWidth: row.implicitWidth + 20
    color: Qt.alpha(current.color, 0.14)
    border.width: 1
    border.color: Qt.alpha(current.color, 0.45)

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 6

        BusyIndicator {
            visible: root.spinning
            running: root.spinning && root.visible
            implicitWidth: 12
            implicitHeight: 12
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: root.labelText()
            color: root.current.color
            font.pixelSize: 12
            font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
