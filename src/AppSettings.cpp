#include "AppSettings.h"

#include <QSettings>

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
{
    QSettings s;
    m_dark = s.value(QStringLiteral("ui/darkTheme"), true).toBool();
    applyPalette();
}

void AppSettings::setDarkTheme(bool dark)
{
    if (m_dark == dark)
        return;
    m_dark = dark;
    QSettings s;
    s.setValue(QStringLiteral("ui/darkTheme"), dark);
    applyPalette();
    emit darkThemeChanged();
}

void AppSettings::applyPalette()
{
    if (m_dark) {
        m_bg         = QColor(0x14, 0x16, 0x1b);
        m_surface    = QColor(0x1d, 0x21, 0x29);
        m_surfaceAlt = QColor(0x26, 0x2b, 0x35);
        m_border     = QColor(0x2e, 0x34, 0x40);
        m_text       = QColor(0xe6, 0xe9, 0xef);
        m_dim        = QColor(0x8b, 0x91, 0xa0);
        m_accent     = QColor(0x7f, 0xc9, 0x8f);
        m_accentHover= QColor(0x93, 0xd8, 0xa2);
        m_accentText = QColor(0x10, 0x15, 0x10);
        m_green      = QColor(0x5e, 0xc2, 0x6f);
        m_amber      = QColor(0xe2, 0xb9, 0x3b);
        m_orange     = QColor(0xe0, 0x95, 0x5a);
        m_purple     = QColor(0xb6, 0x9f, 0xd8);
        m_red        = QColor(0xe0, 0x6c, 0x75);
    } else {
        m_bg         = QColor(0xf4, 0xf5, 0xf7);
        m_surface    = QColor(0xff, 0xff, 0xff);
        m_surfaceAlt = QColor(0xe9, 0xec, 0xf0);
        m_border     = QColor(0xd8, 0xdd, 0xe4);
        m_text       = QColor(0x1f, 0x24, 0x30);
        m_dim        = QColor(0x68, 0x70, 0x7e);
        m_accent     = QColor(0x2e, 0x9e, 0x57);
        m_accentHover= QColor(0x26, 0x8a, 0x4b);
        m_accentText = QColor(0xff, 0xff, 0xff);
        m_green      = QColor(0x25, 0x96, 0x4c);
        m_amber      = QColor(0xb0, 0x7d, 0x0f);
        m_orange     = QColor(0xc0, 0x5f, 0x21);
        m_purple     = QColor(0x7b, 0x5b, 0xc0);
        m_red        = QColor(0xd1, 0x3d, 0x47);
    }
    emit paletteChanged();
}
