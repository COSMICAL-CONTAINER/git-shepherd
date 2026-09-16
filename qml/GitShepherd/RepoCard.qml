import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: root

    required property int index
    required property string path
    required property string name
    required property string branch
    required property string pinnedBranch
    required property var branches
    required property string state
    required property int behind
    required property int ahead
    required property string lastCommit
    required property string lastCommitTime
    required property string error
    required property bool checked

    signal removeRequested(int index)
    signal branchSwitchRequested(string path, string branch)
    signal pinRequested(string path)
    signal unpinRequested(string path)

    readonly property bool branchBusy: state === "checking" || state === "updating"
                                       || state === "switching"

    implicitHeight: 76
    radius: 10
    color: Theme.surface
    border.width: 1
    border.color: Theme.border

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 8
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        spacing: 12

        CheckBox {
            id: check
            checked: root.checked
            onToggled: root.checked = checked
            Layout.alignment: Qt.AlignVCenter

            indicator: Rectangle {
                implicitWidth: 20
                implicitHeight: 20
                radius: 6
                color: check.checked ? Theme.accent : Theme.surfaceAlt
                border.width: 1
                border.color: check.checked ? Theme.accent : Theme.border

                Text {
                    anchors.centerIn: parent
                    text: "✓"
                    visible: check.checked
                    color: Theme.accentText
                    font.pixelSize: 13
                    font.bold: true
                }
            }
        }

        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true

            RowLayout {
                spacing: 8

                Text {
                    text: root.name
                    color: Theme.text
                    font.pixelSize: 15
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.maximumWidth: 240
                }

                // 分支 chip：点击弹出切换菜单
                Button {
                    id: branchChip
                    visible: root.branch.length > 0
                    enabled: !root.branchBusy
                    flat: true
                    hoverEnabled: true
                    padding: 0
                    implicitHeight: 22

                    background: Rectangle {
                        radius: 5
                        color: branchChip.hovered ? Theme.surfaceAlt : "transparent"
                        border.width: 1
                        border.color: branchChip.hovered ? Theme.dim : Theme.border
                    }

                    contentItem: Text {
                        text: root.branch + (branchChip.hovered ? " ▾" : "")
                        leftPadding: 8
                        rightPadding: 8
                        verticalAlignment: Text.AlignVCenter
                        color: branchChip.hovered ? Theme.text : Theme.dim
                        font.pixelSize: 11
                        font.family: "Consolas"
                    }

                    onClicked: branchMenu.open()
                }

                // 已偏离跟随分支：一键切回
                Button {
                    id: pinBack
                    visible: root.pinnedBranch.length > 0
                             && root.pinnedBranch !== root.branch
                    flat: true
                    hoverEnabled: true
                    padding: 0
                    implicitHeight: 22
                    onClicked: root.branchSwitchRequested(root.path, root.pinnedBranch)

                    background: Rectangle {
                        radius: 5
                        color: pinBack.hovered ? Qt.alpha(Theme.amber, 0.25)
                                               : Qt.alpha(Theme.amber, 0.12)
                        border.width: 1
                        border.color: Qt.alpha(Theme.amber, 0.5)
                    }

                    contentItem: Text {
                        text: "📌 " + root.pinnedBranch
                        leftPadding: 8
                        rightPadding: 8
                        verticalAlignment: Text.AlignVCenter
                        color: Theme.amber
                        font.pixelSize: 11
                        font.family: "Consolas"
                    }
                }

                StatusBadge {
                    state: root.state
                    behind: root.behind
                    ahead: root.ahead
                }

                Item { Layout.fillWidth: true }

                Text {
                    visible: root.behind > 0 && root.state !== "updating"
                    text: "↓ " + root.behind
                    color: Theme.amber
                    font.pixelSize: 14
                    font.bold: true
                }

                Text {
                    visible: root.state === "diverged" && root.ahead > 0
                    text: "↑ " + root.ahead
                    color: Theme.purple
                    font.pixelSize: 12
                }
            }

            RowLayout {
                spacing: 8

                Text {
                    text: root.path
                    color: Theme.dim
                    font.pixelSize: 12
                    font.family: "Consolas"
                    elide: Text.ElideMiddle
                    Layout.maximumWidth: 360
                }

                Text {
                    visible: root.state !== "error"
                        && root.lastCommit.length > 0
                    text: (root.lastCommitTime.length > 0
                           ? root.lastCommitTime + " · "
                           : "") + root.lastCommit
                    color: Theme.dim
                    font.pixelSize: 12
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    visible: root.state === "error"
                    text: root.error
                    color: Theme.red
                    font.pixelSize: 12
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }

        Button {
            id: removeBtn
            flat: true
            hoverEnabled: true
            implicitWidth: 30
            implicitHeight: 30
            Layout.alignment: Qt.AlignVCenter
            onClicked: root.removeRequested(root.index)

            contentItem: Text {
                text: "✕"
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: removeBtn.hovered ? Theme.red : Theme.dim
            }

            background: Rectangle {
                radius: 15
                color: removeBtn.hovered ? Theme.surfaceAlt : "transparent"
            }
        }
    }

    BranchMenu {
        id: branchMenu
        parent: branchChip
        y: branchChip.height + 6
        x: Math.round((branchChip.width - width) / 2)
        repoPath: root.path
        currentBranch: root.branch
        pinnedBranch: root.pinnedBranch
        branches: root.branches

        onSwitchRequested: function(branch) {
            root.branchSwitchRequested(root.path, branch)
        }
        onPinCurrentRequested: root.pinRequested(root.path)
        onUnpinRequested: root.unpinRequested(root.path)
    }
}
