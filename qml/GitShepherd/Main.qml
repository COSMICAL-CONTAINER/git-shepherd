import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: root

    width: 960
    height: 680
    minimumWidth: 800
    minimumHeight: 560
    visible: true
    title: qsTr("git-shepherd")
    color: Theme.bg

    property int currentTab: 0
    property string pendingScanFolder: ""

    readonly property var tabNames: [qsTr("全部")].concat(repoModel.folders)
                                   .concat([qsTr("单独仓库")])
    readonly property bool folderTabActive: currentTab >= 1 && currentTab < tabNames.length - 1
    readonly property string currentFolder: folderTabActive
                                            ? repoModel.folders[currentTab - 1] : ""

    RepoModel {
        id: repoModel
    }

    RepoFilterProxy {
        id: repoProxy
        sourceModel: repoModel
        mode: currentTab === 0 ? "all"
             : (currentTab === root.tabNames.length - 1 ? "standalone" : "folder")
        folderPath: root.currentFolder
    }

    Connections {
        target: repoModel
        function onScanFinished(paths) { scanDialog.openWith(paths, root.pendingScanFolder) }
        function onFoldersChanged() {
            root.currentTab = Math.min(root.currentTab, root.tabNames.length - 1)
        }
    }

    header: Column {
        spacing: 0

        // 顶栏：标题 + 全局操作
        Rectangle {
            color: Theme.surface
            implicitHeight: 58
            width: parent.width

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 10

                Text {
                    text: "🐑 git-shepherd"
                    color: Theme.text
                    font.pixelSize: 17
                    font.bold: true
                }

                Text {
                    text: "v0.4"
                    color: Theme.dim
                    font.pixelSize: 11
                    Layout.alignment: Qt.AlignBottom
                    Layout.bottomMargin: 20
                }

                Item { Layout.fillWidth: true }

                AppButton {
                    kind: "accent"
                    text: qsTr("扫描文件夹")
                    onClicked: scanSourceDialog.open()
                }

                AppButton {
                    kind: "outline"
                    text: qsTr("添加仓库")
                    onClicked: addRepoDialog.open()
                }

                AppButton {
                    kind: "ghost"
                    text: qsTr("↻ 检测")
                    enabled: !repoModel.busy && repoModel.totalCount > 0
                    onClicked: repoModel.refreshAll()
                }

                AppButton {
                    id: settingsButton
                    kind: "ghost"
                    text: qsTr("⚙")
                    onClicked: settingsMenu.open()
                }
            }
        }

        // Tab 栏：全部 / 各文件夹 / 单独仓库
        Rectangle {
            color: Theme.surface
            implicitHeight: 44
            width: parent.width

            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: Theme.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8

                Row {
                    spacing: 6
                    Layout.alignment: Qt.AlignVCenter

                    Repeater {
                        model: root.tabNames

                        delegate: Rectangle {
                            id: pill
                            required property string modelData
                            required property int index
                            readonly property bool isFolderTab: index >= 1
                                                                 && index < root.tabNames.length - 1
                            readonly property bool selected: root.currentTab === index
                            // 文件夹 tab 只显示目录名，悬停可通过 ✕ 确认
                            readonly property string label: isFolderTab
                                ? pill.modelData.substring(pill.modelData.lastIndexOf("/") + 1)
                                : pill.modelData

                            height: 30
                            width: pillRow.implicitWidth + 22
                                     + (isFolderTab && pillMouse.containsMouse ? 18 : 0)
                            radius: 15
                            color: selected ? Theme.surfaceAlt : "transparent"
                            border.width: selected ? 1 : 0
                            border.color: Theme.border

                            Row {
                                id: pillRow
                                anchors.centerIn: parent
                                spacing: 4

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: pill.label
                                    color: pill.selected ? Theme.accent : Theme.dim
                                    font.pixelSize: 13
                                    font.bold: pill.selected
                                    elide: Text.ElideMiddle
                                }

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: pill.isFolderTab && pillMouse.containsMouse
                                    text: "✕"
                                    color: closeMouse.containsMouse ? Theme.red : Theme.dim
                                    font.pixelSize: 11

                                    MouseArea {
                                        id: closeMouse
                                        anchors.fill: parent
                                        anchors.margins: -6
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: repoModel.removeFolder(pill.modelData)
                                    }
                                }
                            }

                            MouseArea {
                                id: pillMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.currentTab = pill.index
                            }
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                AppButton {
                    visible: root.folderTabActive
                    kind: "ghost"
                    text: qsTr("↻ 重新扫描此文件夹")
                    onClicked: {
                        root.pendingScanFolder = root.currentFolder
                        repoModel.scanFolder(root.currentFolder)
                    }
                }
            }
        }
    }

    Item {
        anchors.fill: parent

        ListView {
            id: listView
            anchors.fill: parent
            anchors.margins: 16
            spacing: 8
            clip: true
            model: repoProxy
            visible: repoModel.totalCount > 0

            delegate: RepoCard {
                width: ListView.view.width

                onRemoveRequested: function(index) { repoModel.removeAt(index) }
                onBranchSwitchRequested: function(path, ref, kind) {
                    repoModel.switchBranch(path, ref, kind)
                }
                onPinRequested: function(path) { repoModel.pinCurrentBranch(path) }
                onUnpinRequested: function(path) { repoModel.unpinBranch(path) }
            }

            ScrollBar.vertical: ScrollBar {
                contentItem: Rectangle {
                    color: Theme.border
                    radius: 4
                    implicitWidth: 8
                }
            }
        }

        // 全局空列表引导
        ColumnLayout {
            anchors.centerIn: parent
            visible: repoModel.totalCount === 0
            spacing: 14
            width: 460

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "🐑"
                font.pixelSize: 56
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("还没有登记任何仓库")
                color: Theme.text
                font.pixelSize: 18
                font.bold: true
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("扫描一个文件夹，把里面散落的 git 仓库批量登记进来；也可以直接添加单个仓库")
                color: Theme.dim
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }

            AppButton {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 8
                kind: "accent"
                text: qsTr("扫描文件夹…")
                onClicked: scanSourceDialog.open()
            }
        }

        // 当前 tab 为空的提示
        ColumnLayout {
            anchors.centerIn: parent
            visible: repoModel.totalCount > 0 && listView.count === 0
            spacing: 8

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: root.folderTabActive
                      ? qsTr("此文件夹下没有登记的仓库")
                      : qsTr("还没有单独添加的仓库")
                color: Theme.dim
                font.pixelSize: 14
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                visible: root.folderTabActive
                text: qsTr("新 clone 了项目？点右上「↻ 重新扫描此文件夹」")
                color: Theme.dim
                font.pixelSize: 12
            }
        }
    }

    footer: Rectangle {
        color: Theme.surface
        implicitHeight: 60

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Theme.border
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 12

            CheckBox {
                text: qsTr("全选")
                enabled: repoModel.totalCount > 0
                checked: repoModel.totalCount > 0
                         && repoModel.checkedCount === repoModel.totalCount
                onClicked: repoModel.setAllChecked(checked)

                indicator: Rectangle {
                    implicitWidth: 18
                    implicitHeight: 18
                    radius: 5
                    anchors.verticalCenter: parent.verticalCenter
                    color: parent.checked ? Theme.accent : Theme.surfaceAlt
                    border.width: 1
                    border.color: parent.checked ? Theme.accent : Theme.border

                    Text {
                        anchors.centerIn: parent
                        text: "✓"
                        visible: parent.parent.checked
                        color: Theme.accentText
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                contentItem: Text {
                    text: parent.text
                    leftPadding: parent.indicator.width + parent.spacing
                    verticalAlignment: Text.AlignVCenter
                    color: Theme.text
                    font.pixelSize: 13
                }
            }

            Text {
                text: repoModel.summary
                color: Theme.dim
                font.pixelSize: 12
                elide: Text.ElideRight
                Layout.fillWidth: true
                Layout.leftMargin: 4
            }

            BusyIndicator {
                visible: repoModel.busy
                running: repoModel.busy
                implicitWidth: 20
                implicitHeight: 20
                Layout.rightMargin: 4
            }

            AppButton {
                kind: "accent"
                text: qsTr("更新选中 (%1)").arg(repoModel.checkedCount)
                enabled: repoModel.checkedCount > 0 && !repoModel.busy
                onClicked: repoModel.updateChecked()
            }
        }
    }

    FolderDialog {
        id: scanSourceDialog
        title: qsTr("选择要扫描的文件夹")
        onAccepted: {
            root.pendingScanFolder = String(selectedFolder)
            repoModel.scanFolder(root.pendingScanFolder)
        }
    }

    FolderDialog {
        id: addRepoDialog
        title: qsTr("选择要添加的 git 仓库")
        onAccepted: repoModel.addRepo(String(selectedFolder))
    }

    ScanDialog {
        id: scanDialog
        onImportRequested: function(paths, folder) {
            repoModel.importPaths(paths, folder)
        }
    }

    SettingsMenu {
        id: settingsMenu
        parent: settingsButton
        y: settingsButton.height + 10
        x: settingsButton.width - width + 10
    }
}
