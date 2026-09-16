import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Dialog {
    id: root

    property int selectedCount: 0
    property string sourceFolder: ""

    signal importRequested(var paths, string folder)

    function openWith(paths, folder) {
        foundModel.clear()
        for (var i = 0; i < paths.length; ++i)
            foundModel.append({ path: paths[i], picked: true })
        sourceFolder = folder
        recount()
        open()
    }

    function recount() {
        var n = 0
        for (var i = 0; i < foundModel.count; ++i)
            if (foundModel.get(i).picked)
                ++n
        selectedCount = n
    }

    function collect() {
        var out = []
        for (var i = 0; i < foundModel.count; ++i)
            if (foundModel.get(i).picked)
                out.push(foundModel.get(i).path)
        return out
    }

    modal: true
    width: 680
    height: 500
    parent: Overlay.overlay
    anchors.centerIn: parent
    padding: 0

    background: Rectangle {
        color: Theme.surface
        radius: 12
        border.width: 1
        border.color: Theme.border
    }

    header: Item {
        implicitHeight: headerLayout.implicitHeight + 32

        ColumnLayout {
            id: headerLayout
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 20
            spacing: 8

        Text {
            text: foundModel.count > 0
                  ? qsTr("发现 %1 个新 git 仓库").arg(foundModel.count)
                  : qsTr("没有发现新的 git 仓库")
            color: Theme.text
            font.pixelSize: 16
            font.bold: true
        }

        Text {
            visible: foundModel.count === 0
            Layout.fillWidth: true
            text: qsTr("所选文件夹下（含 3 层子目录）没有找到未登记的 git 仓库")
            color: Theme.dim
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }

        RowLayout {
            visible: foundModel.count > 0
            spacing: 10

            CheckBox {
                text: qsTr("全选")
                checked: root.selectedCount === foundModel.count
                onClicked: {
                    for (var i = 0; i < foundModel.count; ++i)
                        foundModel.setProperty(i, "picked", checked)
                    root.recount()
                }
                contentItem: Text {
                    text: parent.text
                    leftPadding: parent.indicator.width + parent.spacing
                    verticalAlignment: Text.AlignVCenter
                    color: Theme.text
                    font.pixelSize: 13
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: qsTr("已选 %1 / %2").arg(root.selectedCount).arg(foundModel.count)
                color: Theme.dim
                font.pixelSize: 12
            }
        }
    }
    }

    contentItem: ListView {
        visible: foundModel.count > 0
        clip: true
        spacing: 2
        leftMargin: 12
        rightMargin: 12
        topMargin: 4
        bottomMargin: 12
        model: ListModel { id: foundModel }

        ScrollBar.vertical: ScrollBar {
            contentItem: Rectangle {
                color: Theme.border
                radius: 4
                implicitWidth: 8
            }
        }

        delegate: CheckBox {
            id: pickRow
            required property string path
            required property bool picked
            required property int index
            width: ListView.view.width
            checked: picked
            onToggled: {
                picked = checked
                root.recount()
            }

            indicator: Rectangle {
                implicitWidth: 18
                implicitHeight: 18
                radius: 5
                anchors.verticalCenter: parent.verticalCenter
                color: pickRow.checked ? Theme.accent : Theme.surfaceAlt
                border.width: 1
                border.color: pickRow.checked ? Theme.accent : Theme.border

                Text {
                    anchors.centerIn: parent
                    text: "✓"
                    visible: pickRow.checked
                    color: Theme.accentText
                    font.pixelSize: 12
                    font.bold: true
                }
            }

            contentItem: Text {
                text: pickRow.path
                leftPadding: pickRow.indicator.width + pickRow.spacing
                verticalAlignment: Text.AlignVCenter
                color: Theme.text
                font.pixelSize: 13
                font.family: "Consolas"
                elide: Text.ElideMiddle
            }

            background: Rectangle {
                radius: 6
                color: pickRow.hovered ? Theme.surfaceAlt : "transparent"
            }
        }
    }

    footer: Item {
        implicitHeight: footerLayout.implicitHeight + 26

        RowLayout {
            id: footerLayout
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            anchors.topMargin: 8
            anchors.bottomMargin: 18
            spacing: 10

            Item { Layout.fillWidth: true }

            AppButton {
                kind: "ghost"
                text: qsTr("取消")
                onClicked: root.close()
            }

            AppButton {
                kind: "accent"
                visible: foundModel.count > 0
                enabled: root.selectedCount > 0
                text: qsTr("导入 %1 个").arg(root.selectedCount)
            onClicked: {
                root.importRequested(root.collect(), root.sourceFolder)
                root.close()
            }
            }
        }
    }
}
