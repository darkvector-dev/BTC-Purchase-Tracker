#pragma once

#include <QSettings>
#include <QString>

namespace AppLanguage {

enum class Language {
    Italian,
    English
};

inline Language g_currentLanguage = Language::Italian;

inline void load() {
    QSettings settings("BTCPurchaseTracker", "BTCPurchaseTracker");
    const QString code = settings.value("ui/language", "it").toString().toLower();
    g_currentLanguage = (code == "en") ? Language::English : Language::Italian;
}

inline Language current() {
    return g_currentLanguage;
}

inline void setCurrent(Language language) {
    g_currentLanguage = language;

    QSettings settings("BTCPurchaseTracker", "BTCPurchaseTracker");
    settings.setValue(
        "ui/language",
        language == Language::English ? QStringLiteral("en") : QStringLiteral("it")
    );
    settings.sync();
}

inline bool isEnglish() {
    return current() == Language::English;
}

inline QString text(const char *italian, const char *english) {
    return QString::fromUtf8(isEnglish() ? english : italian);
}

} // namespace AppLanguage
