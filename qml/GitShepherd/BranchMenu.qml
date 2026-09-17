import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Popup {
    id: root

    property string repoPath: ""
    property string currentBranch: ""
    property string pinnedBranch: ""
    property var branches: []
    property var tags: []

    signal switchRequested(string ref, string kind)
    signal pinCurrentRequested()
    signal unpinRequested()

    // 本地分支名可含斜杠（feature/xxx），只有 origin/ 前缀的才是远端跟踪分支；
    // 其他远端（upstream/*）归入本地区，git checkout 对其同样做 DWIM 建分支
    readonly property var displayList: {
        const locals = branches.filter(b => !b.startsWith("origin/"))
        const remotes = branches.filter(
            b => b.startsWith("origin/") && !locals.includes(b.substring(7)))
        return locals.concat(remotes)
    }
    // 当前是否停在某条分支上（而非 detached / 标签）
    readonly property bool onBranch: currentBranch.length > 0
                                     && currentBranch !== "(detached)"
                                     && !currentBranch.startsWith("🏷")

    width: 264
    padding: 8
    margins: 8
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // 弹层边界防护：底部放不下则向上弹，左缘不越出窗口
    onAboutToShow: {
        if (!parent || !Overlay.overlay)
            return
        const pos = parent.mapToItem(Overlay.overlay, 0, 0)
        y = pos.y + parent.height + height > Overlay.overlay.height - 8
            ? -height - 6 : parent.height + 6
        x = Math.max(8 - pos.x, Math.round((parent.width - width) / 2))
    }
    background: Rectangle {
        color: Theme.surface
        radius: 10
        border.width: 1
        border.color: Theme.border
    }

    contentItem: ColumnLayout {
        spacing: 6

        Text {
            visible: root.displayList.length === 0 && root.tags.length === 0
            Layout.fillWidth: true
            text: qsTr("暂无分支 / 标签信息（先执行一次检测）")
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
            implicitHeight: Math.min(contentHeight, 200)
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
                readonly property bool isRemote: modelData.startsWith("origin/")

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
                            root.switchRequested(branchRow.modelData,
                                                 branchRow.isRemote ? "remote" : "")
                            root.close()
                        }
                    }
                }
            }
        }

        // 标签区
        Rectangle {
            visible: root.tags.length > 0
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.border
        }

        Text {
            visible: root.tags.length > 0
            text: qsTr("标签")
            color: Theme.dim
            font.pixelSize: 11
            leftPadding: 10
        }

        ListView {
            id: tagList
            visible: root.tags.length > 0
            Layout.fillWidth: true
            implicitHeight: Math.min(contentHeight, 140)
            clip: true
            spacing: 2
            model: root.tags

            ScrollBar.vertical: ScrollBar {
                contentItem: Rectangle {
                    color: Theme.border
                    radius: 4
                    implicitWidth: 6
                }
            }

            delegate: Rectangle {
                id: tagRow
                required property string modelData
                readonly property bool isCurrent:
                    root.currentBranch === "🏷 " + tagRow.modelData

                width: tagList.width
                height: 28
                radius: 6
                color: tagRowMouse.containsMouse ? Theme.surfaceAlt : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 8

                    Text {
                        text: tagRow.isCurrent ? "✓" : ""
                        color: Theme.accent
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 12
                    }

                    Text {
                        text: "🏷 " + tagRow.modelData
                        color: Theme.purple
                        font.pixelSize: 13
                        font.family: "Consolas"
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                MouseArea {
                    id: tagRowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (!tagRow.isCurrent) {
                            root.switchRequested(tagRow.modelData, "tag")
                            root.close()
                        }
                    }
                }
            }
        }

        Rectangle {
            visible: root.displayList.length > 0
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.border
        }

        // 跟随分支操作行（detached / 停在标签上时不允许设新跟随，但仍可切回）
        Rectangle {
            visible: root.displayList.length > 0
                     && (root.pinnedBranch.length > 0 || root.onBranch)
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            radius: 6
            color: pinMouse.containsMouse ? Theme.surfaceAlt : "transparent"

            Text {
                anchors.centerIn: parent
                width: parent.width - 16
                text: {
                    if (root.pinnedBranch === root.currentBranch && root.onBranch)
                        return qsTr("📌 已跟随 %1 · 点击取消").arg(root.currentBranch)
                    if (root.pinnedBranch.length > 0)
                        return qsTr("📌 切回跟随分支 %1").arg(root.pinnedBranch)
                    return qsTr("📌 跟随当前分支（%1）").arg(root.currentBranch)
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
                    if (root.pinnedBranch === root.currentBranch && root.onBranch)
                        root.unpinRequested()
                    else if (root.pinnedBranch.length > 0)
                        root.switchRequested(root.pinnedBranch, "")
                    else
                        root.pinCurrentRequested()
                    root.close()
                }
            }
        }
    }
}
