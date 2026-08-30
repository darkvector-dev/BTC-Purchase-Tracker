#pragma once

#include <QLocale>
#include <QString>

namespace AppCurrency {

enum class Currency {
    Euro,
    UsDollar
};

inline QString code(Currency currency) {
    return currency == Currency::UsDollar
        ? QStringLiteral("USD")
        : QStringLiteral("EUR");
}

inline QString symbol(Currency currency) {
    return currency == Currency::UsDollar
        ? QStringLiteral("$")
        : QString::fromUtf8("€");
}

inline QLocale locale(Currency currency) {
    return currency == Currency::UsDollar
        ? QLocale(QLocale::English, QLocale::UnitedStates)
        : QLocale(QLocale::Italian, QLocale::Italy);
}

inline QString formatMajor(double amount, Currency currency, int decimals = 2) {
    const QString number = locale(currency).toString(amount, 'f', decimals);
    return currency == Currency::UsDollar
        ? symbol(currency) + number
        : number + QStringLiteral(" ") + symbol(currency);
}

inline QString formatMoney(qint64 cents, Currency currency) {
    return formatMajor(cents / 100.0, currency, 2);
}

inline QString plainAmount(qint64 cents, Currency currency) {
    QString value = QString::number(cents / 100.0, 'f', 2);
    if (currency == Currency::Euro)
        value.replace('.', ',');
    return value;
}

inline QString pricePerBtcUnit(Currency currency) {
    return symbol(currency) + QStringLiteral("/BTC");
}

} // namespace AppCurrency
