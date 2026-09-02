#include "csvutils.h"
#include "language.h"

#include <QFile>
#include <QLocale>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>
#include <QStringConverter>
#include <limits>

namespace {
QString L(const char *italian, const char *english) {
    return AppLanguage::text(italian, english);
}
}

namespace CsvUtils {

QStringList parseLine(const QString &line, QChar delimiter) {
    QStringList cells;
    QString cell;
    bool quoted = false;
    for (qsizetype i = 0; i < line.size(); ++i) {
        const QChar c = line[i];
        if (c == '"') {
            if (quoted && i + 1 < line.size() && line[i + 1] == '"') {
                cell += '"';
                ++i;
            } else {
                quoted = !quoted;
            }
        } else if (c == delimiter && !quoted) {
            cells << cell.trimmed();
            cell.clear();
        } else {
            cell += c;
        }
    }
    cells << cell.trimmed();
    return cells;
}

QChar detectDelimiter(const QString &header) {
    const QList<QChar> candidates = {';', ',', '\t'};
    QChar best = ';';
    int bestCount = -1;
    for (QChar d : candidates) {
        int count = parseLine(header, d).size();
        if (count > bestCount) { bestCount = count; best = d; }
    }
    return best;
}

QString normalizeHeader(QString s) {
    s = s.trimmed().toLower();
    s.replace('_', ' ');
    s.replace('-', ' ');
    s.replace(QRegularExpression("\\s+"), " ");
    return s;
}

QDate parseDate(QString s) {
    s = s.trimmed();
    const QStringList formats = {"dd/MM/yyyy", "yyyy-MM-dd", "dd-MM-yyyy", "d/M/yyyy", "d-M-yyyy"};
    for (const auto &fmt : formats) {
        QDate d = QDate::fromString(s, fmt);
        if (d.isValid()) return d;
    }
    return {};
}

bool parseMoneyCents(QString s, AppCurrency::Currency currency, qint64 *out) {
    s = s.trimmed();

    // Rifiuta esplicitamente un simbolo/codice di valuta diverso da quello
    // scelto per il database, così un importo non può essere reinterpretato
    // accidentalmente come EUR o USD.
    const QString lower = s.toLower();
    if (currency == AppCurrency::Currency::Euro) {
        if (s.contains('$') || lower.contains("usd") || lower.contains("dollar"))
            return false;
        s.remove(QChar(u'€'));
        s.remove(QRegularExpression("(?i)\\b(eur|euros?)\\b"));
    } else {
        if (s.contains(QChar(u'€')) || lower.contains("eur") || lower.contains("euro"))
            return false;
        s.remove('$');
        s.remove(QRegularExpression("(?i)\\b(usd|us\\s*dollars?|dollars?)\\b"));
    }

    s.remove(QRegularExpression("\\s"));
    if (s.startsWith('+')) s.remove(0, 1);
    if (s.isEmpty() || s.startsWith('-')) return false;
    if (!QRegularExpression("^[0-9.,]+$").match(s).hasMatch()) return false;

    const int commaCount = s.count(',');
    const int dotCount = s.count('.');
    const int lastComma = s.lastIndexOf(',');
    const int lastDot = s.lastIndexOf('.');

    QChar decimalSep;
    QChar thousandsSep;

    if (commaCount > 0 && dotCount > 0) {
        // Con entrambi i separatori, l'ultimo è il separatore decimale.
        decimalSep = lastComma > lastDot ? QChar(',') : QChar('.');
        thousandsSep = decimalSep == QChar(',') ? QChar('.') : QChar(',');
    } else if (commaCount > 0 || dotCount > 0) {
        const QChar sep = commaCount > 0 ? QChar(',') : QChar('.');
        const int count = s.count(sep);
        const int last = s.lastIndexOf(sep);
        const int digitsAfter = s.size() - last - 1;

        if (count == 1) {
            if (digitsAfter == 1 || digitsAfter == 2) {
                decimalSep = sep;
            } else if (digitsAfter == 3) {
                // 1.234 / 1,234 viene interpretato come separatore migliaia.
                thousandsSep = sep;
            } else {
                return false;
            }
        } else {
            // Più separatori uguali: accettiamo solo gruppi da migliaia.
            const QStringList groups = s.split(sep);
            if (groups.isEmpty() || groups.first().isEmpty() || groups.first().size() > 3)
                return false;
            for (int i = 0; i < groups.size(); ++i) {
                if (!QRegularExpression("^\\d+$").match(groups[i]).hasMatch())
                    return false;
                if (i > 0 && groups[i].size() != 3)
                    return false;
            }
            thousandsSep = sep;
        }
    }

    if (!thousandsSep.isNull())
        s.remove(thousandsSep);

    if (!decimalSep.isNull()) {
        if (s.count(decimalSep) != 1) return false;
        s.replace(decimalSep, '.');
    }

    if (!QRegularExpression("^\\d+(?:\\.\\d{1,2})?$").match(s).hasMatch())
        return false;

    bool ok = false;
    const double val = s.toDouble(&ok);
    if (!ok || val < 0.0) return false;
    *out = qRound64(val * 100.0);
    return true;
}

bool parseSats(QString s, qint64 *out) {
    s = s.trimmed();

    // Accetta anche valori frazionari di satoshi mostrati da alcuni servizi
    // (es. Lightning/custodial) e li arrotonda al satoshi intero piu' vicino.
    // Formati supportati, per esempio:
    //   78489
    //   78489,696901
    //   78489.696901
    //   3,622.323
    //   3.622,323
    //
    // Se sono presenti sia punto sia virgola, l'ultimo dei due e' considerato
    // il separatore decimale e l'altro il separatore delle migliaia.

    s.remove(QRegularExpression("(?i)\\s*(sats?|satoshi)\\s*$"));
    s.remove(QRegularExpression("\\s"));
    s = s.trimmed();

    if (s.startsWith('+')) s.remove(0, 1);
    if (s.isEmpty() || s.startsWith('-')) return false;

    const int lastComma = s.lastIndexOf(',');
    const int lastDot = s.lastIndexOf('.');

    QChar decimalSep;
    QChar thousandsSep;

    if (lastComma >= 0 && lastDot >= 0) {
        if (lastComma > lastDot) {
            decimalSep = ',';
            thousandsSep = '.';
        } else {
            decimalSep = '.';
            thousandsSep = ',';
        }
    } else if (lastComma >= 0 || lastDot >= 0) {
        const QChar sep = lastComma >= 0 ? QChar(',') : QChar('.');
        const int last = s.lastIndexOf(sep);
        const int digitsAfter = s.size() - last - 1;
        const int digitsBefore = last;

        // Con un solo separatore c'e' un caso ambiguo: "78.489".
        // Se ci sono esattamente 3 cifre dopo e al massimo 3 prima,
        // lo interpretiamo come separatore delle migliaia (78.489 -> 78489).
        // Negli altri casi lo interpretiamo come separatore decimale
        // (3622.323 -> 3622,323 sat; 78489,696901 -> 78489,696901 sat).
        if (s.count(sep) == 1 && digitsAfter == 3 && digitsBefore <= 3) {
            thousandsSep = sep;
        } else if (s.count(sep) == 1) {
            decimalSep = sep;
        } else {
            // Piu' separatori uguali: accettiamo solo gruppi da migliaia
            // del tipo 1.234.567 / 1,234,567.
            const QStringList groups = s.split(sep);
            if (groups.isEmpty() || groups.first().isEmpty() || groups.first().size() > 3)
                return false;
            for (int i = 0; i < groups.size(); ++i) {
                if (!QRegularExpression("^\\d+$").match(groups[i]).hasMatch())
                    return false;
                if (i > 0 && groups[i].size() != 3)
                    return false;
            }
            thousandsSep = sep;
        }
    }

    if (!thousandsSep.isNull())
        s.remove(thousandsSep);

    QString wholePart = s;
    QString fracPart;

    if (!decimalSep.isNull()) {
        if (s.count(decimalSep) != 1) return false;
        const int pos = s.indexOf(decimalSep);
        wholePart = s.left(pos);
        fracPart = s.mid(pos + 1);
    }

    if (wholePart.isEmpty()) wholePart = "0";

    if (!QRegularExpression("^\\d+$").match(wholePart).hasMatch())
        return false;
    if (!fracPart.isEmpty() &&
        !QRegularExpression("^\\d+$").match(fracPart).hasMatch())
        return false;

    bool ok = false;
    qint64 whole = wholePart.toLongLong(&ok);
    if (!ok || whole < 0) return false;

    // Arrotondamento "half up" al satoshi intero piu' vicino.
    // Basta guardare la prima cifra decimale:
    // 0,499... -> giu'; 0,500... -> su.
    if (!fracPart.isEmpty() && fracPart.front() >= QChar('5')) {
        if (whole == std::numeric_limits<qint64>::max()) return false;
        ++whole;
    }

    *out = whole;
    return true;
}

bool parseBtcToSats(QString s, qint64 *out) {
    s = s.trimmed().toLower();
    s.remove("btc");
    s.remove(' ');
    s.replace(',', '.');
    if (s.startsWith('+')) s.remove(0,1);
    if (s.startsWith('-') || s.isEmpty()) return false;
    if (s.count('.') > 1) return false;
    const QStringList parts = s.split('.');
    bool okInt = false;
    qint64 whole = parts[0].isEmpty() ? 0 : parts[0].toLongLong(&okInt);
    if (parts[0].isEmpty()) okInt = true;
    if (!okInt || whole < 0) return false;
    QString frac = parts.size() > 1 ? parts[1] : QString();
    if (frac.size() > 8 || !QRegularExpression("^\\d*$").match(frac).hasMatch()) return false;
    frac = frac.leftJustified(8, '0');
    bool okFrac = false;
    qint64 fractional = frac.isEmpty() ? 0 : frac.toLongLong(&okFrac);
    if (frac.isEmpty()) okFrac = true;
    if (!okFrac) return false;
    if (whole > (std::numeric_limits<qint64>::max() - fractional) / 100000000LL) return false;
    *out = whole * 100000000LL + fractional;
    return true;
}

QString satsToBtc(qint64 sats) {
    const qint64 whole = sats / 100000000LL;
    const qint64 frac = sats % 100000000LL;
    return QString("%1.%2").arg(whole).arg(frac, 8, 10, QChar('0'));
}

QString formatMoney(qint64 cents, AppCurrency::Currency currency) {
    return AppCurrency::formatMoney(cents, currency);
}

QString formatSats(qint64 sats) {
    QLocale it(QLocale::Italian, QLocale::Italy);
    return it.toString(sats);
}

QString csvEscape(const QString &s, QChar delimiter) {
    QString out = s;
    const bool needs = out.contains(delimiter) || out.contains('"') || out.contains('\n') || out.contains('\r');
    out.replace("\"", "\"\"");
    return needs ? QString("\"%1\"").arg(out) : out;
}

CsvImportResult importFile(const QString &path, const Database &db) {
    CsvImportResult result;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.errors << L("Impossibile aprire il file: %1", "Unable to open the file: %1").arg(f.errorString());
        return result;
    }
    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);
    if (in.atEnd()) { result.errors << L("Il CSV è vuoto.", "The CSV is empty."); return result; }

    QString headerLine = in.readLine();
    if (!headerLine.isEmpty() && headerLine.front() == QChar(0xFEFF)) headerLine.remove(0,1);
    const QChar delimiter = detectDelimiter(headerLine);
    const QStringList headers = parseLine(headerLine, delimiter);

    QMap<QString,int> idx;
    for (int i=0;i<headers.size();++i) idx[normalizeHeader(headers[i])] = i;

    auto find = [&](const QStringList &names)->int {
        for (const auto &n : names) {
            const auto key = normalizeHeader(n);
            if (idx.contains(key)) return idx[key];
        }
        return -1;
    };

    const int iDate = find({"data", "date", "purchase date", "data acquisto"});
    const int iSite = find({"sito", "site", "exchange", "piattaforma", "sito / exchange", "site / exchange"});
    const int iEuro = find({"euro", "eur", "euro spesi", "importo euro", "amount eur", "euro spent", "eur spent"});
    const int iUsd  = find({"usd", "dollari", "dollari spesi", "dollari spesi (usd)", "importo usd", "importo dollari", "amount usd", "usd spent", "us dollars spent", "dollars spent"});
    const int iBtc  = find({"btc", "bitcoin", "btc on chain", "btc on-chain"});
    const int iSats = find({"satoshi", "sats", "sat"});
    const int iTx   = find({"tx", "txid", "transaction id", "id transazione", "tx / id transazione", "tx / transaction id"});

    const bool useUsd = db.currency() == AppCurrency::Currency::UsDollar;
    const int iAmount = useUsd ? iUsd : iEuro;
    const int iWrongCurrency = useUsd ? iEuro : iUsd;

    if (iAmount < 0 && iWrongCurrency >= 0) {
        result.errors << (AppLanguage::isEnglish()
            ? QString("Currency mismatch: this database uses %1, but the CSV declares %2.")
                  .arg(AppCurrency::code(db.currency()), useUsd ? QStringLiteral("EUR") : QStringLiteral("USD"))
            : QString("Valuta non corrispondente: questo database usa %1, ma il CSV dichiara %2.")
                  .arg(AppCurrency::code(db.currency()), useUsd ? QStringLiteral("EUR") : QStringLiteral("USD")));
        return result;
    }

    if (iDate < 0 || iSite < 0 || iAmount < 0 || (iBtc < 0 && iSats < 0)) {
        result.errors << (AppLanguage::isEnglish()
            ? QString("Unrecognized headers. Date, Site/Exchange, %1 and at least BTC or Satoshi are required.")
                  .arg(AppCurrency::code(db.currency()))
            : QString("Intestazioni non riconosciute. Servono Data, Sito/Exchange, %1 e almeno BTC oppure Satoshi.")
                  .arg(AppCurrency::code(db.currency())));
        return result;
    }

    QSet<QString> seenTx;
    const QDate minimumPurchaseDate(2009, 1, 3);
    const QDate maximumPurchaseDate = QDate::currentDate();
    int lineNo = 1;
    while (!in.atEnd()) {
        ++lineNo;
        const QString line = in.readLine();
        if (line.trimmed().isEmpty()) continue;
        const QStringList cells = parseLine(line, delimiter);
        auto get = [&](int i)->QString { return (i >= 0 && i < cells.size()) ? cells[i].trimmed() : QString(); };

        Purchase p;
        p.date = parseDate(get(iDate));
        p.site = get(iSite);
        p.txid = get(iTx);
        if (p.txid.isNull())
            p.txid = QStringLiteral("");
        QStringList rowErrors;
        if (!p.date.isValid()) {
            rowErrors << L("data non valida", "invalid date");
        } else if (p.date < minimumPurchaseDate || p.date > maximumPurchaseDate) {
            rowErrors << L(
                "data fuori intervallo (dal 03/01/2009 a oggi)",
                "date outside the allowed range (03/01/2009 through today)"
            );
        }
        if (p.site.isEmpty()) rowErrors << L("sito/exchange mancante", "missing site/exchange");
        if (!parseMoneyCents(get(iAmount), db.currency(), &p.euroCents)
            || p.euroCents <= 0) {
            rowErrors << (useUsd
                ? L("dollari non validi", "invalid USD amount")
                : L("euro non validi", "invalid euro amount"));
        }

        qint64 satsFromBtc = -1, satsFromSats = -1;
        bool hasBtc = iBtc >= 0 && !get(iBtc).isEmpty();
        bool hasSats = iSats >= 0 && !get(iSats).isEmpty();
        bool btcOk = !hasBtc || parseBtcToSats(get(iBtc), &satsFromBtc);
        bool satsOk = !hasSats || parseSats(get(iSats), &satsFromSats);
        if (!btcOk) rowErrors << L("BTC non validi", "invalid BTC");
        if (!satsOk) rowErrors << L("satoshi non validi", "invalid satoshi");
        if (!hasBtc && !hasSats) rowErrors << L("BTC/satoshi mancanti", "missing BTC/satoshi");
        if (hasBtc && hasSats && btcOk && satsOk && satsFromBtc != satsFromSats) rowErrors << L("BTC e satoshi non coincidono", "BTC and satoshi do not match");
        if ((hasBtc || hasSats) && btcOk && satsOk
            && (!hasBtc || !hasSats || satsFromBtc == satsFromSats)) {
            p.sats = hasSats ? satsFromSats : satsFromBtc;
            if (p.sats <= 0)
                rowErrors << L("i satoshi devono essere maggiori di zero", "satoshi must be greater than zero");
        }

        if (!p.txid.isEmpty() && (db.txidExists(p.txid) || seenTx.contains(p.txid))) {
            ++result.duplicateRows;
            continue;
        }
        if (!p.txid.isEmpty()) seenTx.insert(p.txid);

        if (!rowErrors.isEmpty()) {
            result.errors << (AppLanguage::isEnglish()
                ? QString("Row %1: %2").arg(lineNo).arg(rowErrors.join(", "))
                : QString("Riga %1: %2").arg(lineNo).arg(rowErrors.join(", ")));
        } else {
            result.validRows.push_back(p);
        }
    }
    return result;
}

} // namespace CsvUtils
