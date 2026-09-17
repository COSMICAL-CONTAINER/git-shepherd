import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: root

    required property string state
    property int behind: 0
    property int ahead: 0

    // 标量属性替代整表 var 对象：依赖只剩 state 与用到的 Theme 令牌，
    // 主题切换时的重求值面最小
    readonly property color badgeColor: {
        switch (state) {
        case "updating":
        case "switching": return Theme.accent
        case "upToDate":
        case "updated": return Theme.green
        case "updateAvailable": return Theme.amber
        case "dirty": return Theme.orange
        case "diverged": return Theme.purple
        case "error": return Theme.red
        default: return Theme.dim
        }
    }
    readonly property bool spinning: state === "checking" || state === "updating"
                                     || state === "switching"

    function labelText(): string {
        switch (state) {
        case "checking": return qsTr("检测中")
        case "updating": return qsTr("更新中")
        case "switching": return qsTr("切换中")
        case "upToDate": return qsTr("最新")
        case "updateAvailable": return qsTr("可更新")
        case "dirty": return qsTr("本地改动")
        case "diverged": return behind > 0 ? qsTr("已分叉") : qsTr("未推送")
        case "noUpstream": return qsTr("无上游")
        case "error": return qsTr("错误")
        case "updated": return qsTr("已更新")
        default: return qsTr("待检测")
        }
    }

    radius: height / 2
    implicitHeight: 22
    implicitWidth: row.implicitWidth + 20
    color: Qt.alpha(badgeColor, 0.14)
    border.width: 1
    border.color: Qt.alpha(badgeColor, 0.45)

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
            color: root.badgeColor
            font.pixelSize: 12
            font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
