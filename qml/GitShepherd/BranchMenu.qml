import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Popup {
    id: root

    property string repoPath: ""
    property string currentBranch: ""
    property string pinnedBranch: ""
    property var branches: []

    signal switchRequested(string branch)
    signal pinCurrentRequested()
    signal unpinRequested()

    // 只展示本地分支 + 远端独有分支（远端去重后带 origin/ 前缀）
    readonly property var displayList: {
        const locals = branches.filter(b => !b.includes("/"))
        const remotes = branches.filter(
            b => b.startsWith("origin/") && !locals.includes(b.substring(7)))
        return locals.concat(remotes)
    }

    width: 264
    padding: 8
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle {
        color: Theme.surface
        radius: 10
        border.width: 1
        border.color: Theme.border
    }

    contentItem: ColumnLayout {
        spacing: 6

        Text {
            visible: root.displayList.length === 0
            Layout.fillWidth: true
            text: qsTr("暂无分支信息（先执行一次检测）")
            color: Theme.dim
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            topPadding: 8
            bottomPadding: 8
        }

        ListView {
            id: branchList
            visible: root.displayList.length > 0
            Layout.fillWidth: true
            implicitHeight: Math.min(contentHeight, 264)
            clip: true
            spacing: 2
            model: root.displayList

            ScrollBar.vertical: ScrollBar {
                contentItem: Rectangle {
                    color: Theme.border
                    radius: 4
                    implicitWidth: 6
                }
            }

            delegate: Rectangle {
                id: branchRow
                required property string modelData
                readonly property bool isCurrent: modelData === root.currentBranch
                readonly property bool isRemote: modelData.includes("/")

                width: branchList.width
                height: 30
                radius: 6
                color: branchRowMouse.containsMouse ? Theme.surfaceAlt : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 8

                    Text {
                        text: branchRow.isCurrent ? "✓" : ""
                        color: Theme.accent
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 12
                    }

                    Text {
                        text: branchRow.modelData
                        color: branchRow.isRemote ? Theme.dim : Theme.text
                        font.pixelSize: 13
                        font.family: "Consolas"
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Text {
                        visible: branchRow.modelData === root.pinnedBranch
                        text: "📌"
                        font.pixelSize: 11
                    }
                }

                MouseArea {
                    id: branchRowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (!branchRow.isCurrent) {
                            root.switchRequested(branchRow.modelData)
                            root.close()
                        }
                    }
                }
            }
        }

        Rectangle {
            visible: root.displayList.length > 0
            Layout.fillWidth: true
            height: 1
            color: Theme.border
        }

        // 跟随分支操作行
        Rectangle {
            visible: root.displayList.length > 0
            Layout.fillWidth: true
            height: 34
            radius: 6
            color: pinMouse.containsMouse ? Theme.surfaceAlt : "transparent"

            Text {
                anchors.centerIn: parent
                width: parent.width - 16
                text: {
                    if (root.pinnedBranch.length === 0)
                        return qsTr("📌 跟随当前分支（%1）").arg(root.currentBranch)
                    if (root.pinnedBranch === root.currentBranch)
                        return qsTr("📌 已跟随 %1 · 点击取消").arg(root.currentBranch)
                    return qsTr("📌 切回跟随分支 %1").arg(root.pinnedBranch)
                }
                color: Theme.dim
                font.pixelSize: 12
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
            }

            MouseArea {
                id: pinMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (root.pinnedBranch.length === 0)
                        root.pinCurrentRequested()
                    else if (root.pinnedBranch === root.currentBranch)
                        root.unpinRequested()
                    else
                        root.switchRequested(root.pinnedBranch)
                    root.close()
                }
            }
        }
    }
}
