#pragma once

#include "database.h"
#include <QString>
#include <QStringList>
#include <QVector>

struct CsvImportResult {
    QVector<Purchase> validRows;
    QStringList errors;
    int duplicateRows = 0;
};

namespace CsvUtils {
QStringList parseLine(const QString &line, QChar delimiter);
QChar detectDelimiter(const QString &header);
QString normalizeHeader(QString s);
QDate parseDate(QString s);
bool parseEuroCents(QString s, qint64 *out);
bool parseSats(QString s, qint64 *out);
bool parseBtcToSats(QString s, qint64 *out);
QString satsToBtc(qint64 sats);
QString formatEuro(qint64 cents);
QString formatSats(qint64 sats);
QString csvEscape(const QString &s, QChar delimiter);
CsvImportResult importFile(const QString &path, const Database &db);
}
