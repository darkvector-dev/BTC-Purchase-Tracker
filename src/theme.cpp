#include "theme.h"
#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QSettings>
#include <QStyleFactory>
#include <QWidget>

namespace {
bool darkTheme = false;
void apply() {
    QPalette p;
    const QColor window(darkTheme ? "#25282c" : "#f3f4f6");
    const QColor base(darkTheme ? "#1b1e22" : "#ffffff");
    const QColor text(darkTheme ? "#edf0f3" : "#202428");
    const QColor muted(darkTheme ? "#aeb6bf" : "#626b75");
    const QColor button(darkTheme ? "#343940" : "#e6e9ed");
    const QColor border(darkTheme ? "#727d89" : "#9ba4ae");
    for (auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        const bool disabled = group == QPalette::Disabled;
        p.setColor(group, QPalette::Window, window);
        p.setColor(group, QPalette::WindowText, disabled ? muted : text);
        p.setColor(group, QPalette::Base, base);
        p.setColor(group, QPalette::AlternateBase, darkTheme ? QColor("#292e34") : QColor("#f0f3f6"));
        p.setColor(group, QPalette::Text, disabled ? muted : text);
        p.setColor(group, QPalette::PlaceholderText, muted);
        p.setColor(group, QPalette::Button, button);
        p.setColor(group, QPalette::ButtonText, disabled ? muted : text);
        p.setColor(group, QPalette::Highlight, disabled ? border : QColor("#f7931a"));
        p.setColor(group, QPalette::HighlightedText, QColor("#171a1e"));
        p.setColor(group, QPalette::ToolTipBase, button);
        p.setColor(group, QPalette::ToolTipText, text);
        p.setColor(group, QPalette::Link, darkTheme ? QColor("#ffb454") : QColor("#955000"));
        p.setColor(group, QPalette::LinkVisited, darkTheme ? QColor("#d6adff") : QColor("#704099"));
        p.setColor(group, QPalette::Light, darkTheme ? QColor("#59616c") : QColor("#ffffff"));
        p.setColor(group, QPalette::Midlight, button);
        p.setColor(group, QPalette::Mid, border);
        p.setColor(group, QPalette::Dark, darkTheme ? QColor("#121519") : QColor("#737d88"));
        p.setColor(group, QPalette::Shadow, QColor("#101215"));
        p.setColor(group, QPalette::BrightText, QColor("#ffffff"));
    }
    QApplication::setPalette(p);
    // Custom painted charts also need repainting immediately after a switch.
    for (auto *widget : QApplication::allWidgets())
        widget->update();
}
}
namespace AppTheme {
bool isDark() { return darkTheme; }
void initialize() {
    // Fusion respects our explicit palettes consistently on all platforms.
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QSettings settings("BTCPurchaseTracker", "BTCPurchaseTracker");
    darkTheme = settings.value("appearance/dark", false).toBool();
    apply();
}
void setDark(bool dark) {
    darkTheme = dark;
    QSettings settings("BTCPurchaseTracker", "BTCPurchaseTracker");
    settings.setValue("appearance/dark", dark);
    apply();
}
}
