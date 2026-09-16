#pragma once

#include <QColor>
#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

// QSettings 支持的少量应用设置 + 调色板。
// 以引擎根上下文属性 "AppSettings" 暴露给 QML（main.cpp 注册）——
// Qt 6.11 mingw 下 QML_SINGLETON 类型对同模块 QML 不可见（ReferenceError），
// 且颜色必须由 C++ NOTIFY 驱动，QML 单例间条件绑定会失去响应。
class AppSettings : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool darkTheme READ darkTheme WRITE setDarkTheme NOTIFY darkThemeChanged)

    Q_PROPERTY(QColor bg READ bg NOTIFY paletteChanged)
    Q_PROPERTY(QColor surface READ surface NOTIFY paletteChanged)
    Q_PROPERTY(QColor surfaceAlt READ surfaceAlt NOTIFY paletteChanged)
    Q_PROPERTY(QColor border READ border NOTIFY paletteChanged)
    Q_PROPERTY(QColor text READ text NOTIFY paletteChanged)
    Q_PROPERTY(QColor dim READ dim NOTIFY paletteChanged)
    Q_PROPERTY(QColor accent READ accent NOTIFY paletteChanged)
    Q_PROPERTY(QColor accentHover READ accentHover NOTIFY paletteChanged)
    Q_PROPERTY(QColor accentText READ accentText NOTIFY paletteChanged)
    Q_PROPERTY(QColor green READ green NOTIFY paletteChanged)
    Q_PROPERTY(QColor amber READ amber NOTIFY paletteChanged)
    Q_PROPERTY(QColor orange READ orange NOTIFY paletteChanged)
    Q_PROPERTY(QColor purple READ purple NOTIFY paletteChanged)
    Q_PROPERTY(QColor red READ red NOTIFY paletteChanged)

public:
    explicit AppSettings(QObject *parent = nullptr);

    bool darkTheme() const { return m_dark; }
    void setDarkTheme(bool dark);

    QColor bg() const { return m_bg; }
    QColor surface() const { return m_surface; }
    QColor surfaceAlt() const { return m_surfaceAlt; }
    QColor border() const { return m_border; }
    QColor text() const { return m_text; }
    QColor dim() const { return m_dim; }
    QColor accent() const { return m_accent; }
    QColor accentHover() const { return m_accentHover; }
    QColor accentText() const { return m_accentText; }
    QColor green() const { return m_green; }
    QColor amber() const { return m_amber; }
    QColor orange() const { return m_orange; }
    QColor purple() const { return m_purple; }
    QColor red() const { return m_red; }

signals:
    void darkThemeChanged();
    void paletteChanged();

private:
    void applyPalette();

    bool m_dark = true;
    QColor m_bg, m_surface, m_surfaceAlt, m_border, m_text, m_dim;
    QColor m_accent, m_accentHover, m_accentText;
    QColor m_green, m_amber, m_orange, m_purple, m_red;
};
