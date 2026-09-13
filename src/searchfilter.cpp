#include "searchfilter.h"

#include "csvutils.h"

#include <QStringList>

namespace {

bool containsText(const QString &value, const QString &query) {
    return value.contains(query, Qt::CaseInsensitive);
}

bool matchesDate(const QDate &date, QString query) {
    if (!date.isValid())
        return false;

    query = query.trimmed();
    QString slashQuery = query;
    slashQuery.replace('-', '/');
    slashQuery.replace('.', '/');

    const QStringList representations{
        date.toString(QStringLiteral("dd/MM/yyyy")),
        date.toString(QStringLiteral("d/M/yyyy")),
        date.toString(QStringLiteral("dd/MM")),
        date.toString(QStringLiteral("d/M")),
        date.toString(QStringLiteral("MM/yyyy")),
        date.toString(QStringLiteral("M/yyyy")),
        date.toString(QStringLiteral("yyyy")),
        date.toString(Qt::ISODate),
        QString(date.toString(Qt::ISODate)).replace('-', '/')
    };

    for (const QString &representation : representations) {
        if (representation.contains(query, Qt::CaseInsensitive)
            || representation.contains(slashQuery, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool matchesAmount(
    const Purchase &purchase,
    const QString &query,
    AppCurrency::Currency currency
) {
    qint64 cents = 0;
    return CsvUtils::parseMoneyCents(query, currency, &cents)
        && cents == purchase.euroCents;
}

bool matchesBitcoin(const Purchase &purchase, QString query) {
    query = query.trimmed();
    const QString lower = query.toLower();
    const bool explicitlyBtc = lower.contains(QStringLiteral("btc"));
    const bool explicitlySats = lower.contains(QStringLiteral("sat"));

    qint64 parsed = 0;

    if (explicitlyBtc)
        return CsvUtils::parseBtcToSats(query, &parsed) && parsed == purchase.sats;

    if (explicitlySats)
        return CsvUtils::parseSats(query, &parsed) && parsed == purchase.sats;

    // Senza suffisso, i valori con separatore decimale sono interpretati come
    // BTC; gli interi come satoshi. "1 BTC" e "1 sat" restano disponibili
    // per risolvere esplicitamente l'unico caso ambiguo.
    if (query.contains('.') || query.contains(','))
        return CsvUtils::parseBtcToSats(query, &parsed) && parsed == purchase.sats;

    return CsvUtils::parseSats(query, &parsed) && parsed == purchase.sats;
}

} // namespace

namespace PurchaseSearch {

bool matches(
    const Purchase &purchase,
    const QString &query,
    Field field,
    AppCurrency::Currency currency
) {
    const QString trimmed = query.trimmed();
    if (trimmed.isEmpty())
        return true;

    switch (field) {
    case Field::Date:
        return matchesDate(purchase.date, trimmed);
    case Field::Site:
        return containsText(purchase.site, trimmed);
    case Field::Amount:
        return matchesAmount(purchase, trimmed, currency);
    case Field::Bitcoin:
        return matchesBitcoin(purchase, trimmed);
    case Field::Transaction:
        return containsText(purchase.txid, trimmed);
    case Field::All:
        return matchesDate(purchase.date, trimmed)
            || containsText(purchase.site, trimmed)
            || matchesAmount(purchase, trimmed, currency)
            || matchesBitcoin(purchase, trimmed)
            || containsText(purchase.txid, trimmed);
    }

    return false;
}

} // namespace PurchaseSearch
