#include "themecontroller.h"

#include <QFontDatabase>
#include <QGuiApplication>
#include <QStyleHints>

ThemeController::ThemeController()
{
#ifdef Q_OS_MAC
    QFontDatabase::addApplicationFont(":/resources/IBMPlexSans-Bold.ttf");
    QFontDatabase::addApplicationFont(":/resources/IBMPlexSans-Regular.ttf");
#endif
}

ThemeController::~ThemeController()
{
}

QString ThemeController::defaultFontName() const
{
    return "IBM Plex Sans";
}

QFont ThemeController::defaultFont(int sizePx, QFont::Weight weight) const
{
    QFont font = QFont(defaultFontName());
    font.setPixelSize(sizePx);
    font.setWeight(weight);
    return font;
}

QColor ThemeController::windowBackgroundColor() const
{
    return QColor(0x02, 0x0D, 0x1C);
}

QColor ThemeController::defaultFontColor() const
{
    return QColor(0xE8, 0xEC, 0xEE);
}

QColor ThemeController::primaryButtonColor() const
{
    return QColor(0x55, 0xFF, 0xA8);
}

QColor ThemeController::primaryButtonHoverColor() const
{
    return QColor(0xFF, 0xFF, 0xFF);
}

QColor ThemeController::primaryButtonFontColor() const
{
    return QColor(0x02, 0x0D, 0x1C);
}

QColor ThemeController::secondaryButtonColor() const
{
    return QColor(0x7F, 0x7F, 0x7F);
}

QColor ThemeController::secondaryButtonHoverColor() const
{
    return QColor(0xFF, 0xFF, 0xFF);
}
