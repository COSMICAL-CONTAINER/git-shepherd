pragma Singleton
import QtQuick

// 调色板实际由 C++ AppSettings 持有，这里只是别名——
// 颜色一律引用 Theme.*，主题切换时全部自动刷新。
QtObject {
    readonly property color bg: AppSettings.bg
    readonly property color surface: AppSettings.surface
    readonly property color surfaceAlt: AppSettings.surfaceAlt
    readonly property color border: AppSettings.border
    readonly property color text: AppSettings.text
    readonly property color dim: AppSettings.dim

    readonly property color accent: AppSettings.accent
    readonly property color accentHover: AppSettings.accentHover
    readonly property color accentText: AppSettings.accentText

    readonly property color green: AppSettings.green
    readonly property color amber: AppSettings.amber
    readonly property color orange: AppSettings.orange
    readonly property color purple: AppSettings.purple
    readonly property color red: AppSettings.red
}
