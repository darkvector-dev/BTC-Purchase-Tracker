#include "csvutils.h"
#include "database.h"
#include "monthlystats.h"
#include "searchfilter.h"

#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringConverter>
#include <QTemporaryDir>
#include <QTextStream>

namespace {
int failures = 0;

void expect(bool condition, const QString &message) {
    if (condition)
        return;
    ++failures;
    QTextStream(stderr) << "FAIL: " << message << '\n';
}

// Read the error only inside this function, after the operation has completed.
// Formatting it in another argument can capture the previous error on MSVC.
void expect(bool condition, const QString &message, const QString &error) {
    expect(condition, message + QStringLiteral(": ") + error);
}

Purchase purchase(const QString &txid, qint64 cents = 1000, qint64 sats = 100) {
    Purchase p;
    p.date = QDate(2026, 1, 1);
    p.site = QStringLiteral("Test Exchange");
    p.euroCents = cents;
    p.sats = sats;
    p.txid = txid;
    return p;
}

bool writeUtf8(const QString &path, const QString &contents) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << contents;
    return stream.status() == QTextStream::Ok;
}

void testDatabaseRulesAndBackup(const QString &root) {
    const QString databasePath = root + QStringLiteral("/current.sqlite");
    Database db;
    QString error;
    expect(db.open(databasePath, &error), QStringLiteral("open new database"), error);
    expect(db.setCurrency(AppCurrency::Currency::Euro, &error),
           QStringLiteral("set database currency"), error);

    expect(db.addPurchase(purchase(QStringLiteral("AbC123")), &error),
           QStringLiteral("insert valid purchase"), error);
    expect(db.txidExists(QStringLiteral("AbC123")), QStringLiteral("find exact TXID"));
    expect(!db.txidExists(QStringLiteral("abc123")), QStringLiteral("TXID comparison is case-sensitive"));
    expect(db.addPurchase(purchase(QStringLiteral("abc123")), &error),
           QStringLiteral("allow TXID differing only by case"), error);

    expect(!db.addPurchase(purchase(QStringLiteral("zero-amount"), 0, 100), &error),
           QStringLiteral("reject zero purchase amount"));
    expect(!db.addPurchase(purchase(QStringLiteral("zero-sats"), 1000, 0), &error),
           QStringLiteral("reject zero satoshi amount"));

    Purchase invalidUpdate = db.purchases().first();
    invalidUpdate.euroCents = 0;
    expect(!db.updatePurchase(invalidUpdate, &error), QStringLiteral("reject zero amount on update"));

    QVector<Purchase> invalidBatch{
        purchase(QStringLiteral("batch-valid")),
        purchase(QStringLiteral("batch-invalid"), 1000, 0)
    };
    expect(!db.addPurchasesTransaction(invalidBatch, &error),
           QStringLiteral("reject transaction containing zero values"));
    expect(db.purchases().size() == 2, QStringLiteral("invalid transaction writes no rows"));

    const QString backupPath = root + QStringLiteral("/backup.sqlite");
    expect(db.backupTo(backupPath, &error), QStringLiteral("create database backup"), error);
    expect(error.isEmpty(), QStringLiteral("successful backup clears previous error"));
    expect(!db.backupTo(databasePath, &error), QStringLiteral("reject backup onto active database"));

    expect(db.addPurchase(purchase(QStringLiteral("after-first-backup")), &error),
           QStringLiteral("insert before replacing backup"), error);
    expect(db.backupTo(backupPath, &error), QStringLiteral("replace existing backup safely"), error);

    Database restored;
    expect(restored.open(backupPath, &error), QStringLiteral("open generated backup"), error);
    expect(restored.purchases(&error).size() == 3,
           QStringLiteral("replacement backup contains the latest rows"), error);
    expect(restored.hasStoredCurrency() && restored.currency() == AppCurrency::Currency::Euro,
           QStringLiteral("backup preserves currency metadata"));
    restored.close();

    // A failed attempt must leave both the existing backup and the source intact.
    db.close();
    expect(!db.backupTo(backupPath, &error), QStringLiteral("reject backup from a closed database"));
    expect(restored.open(backupPath, &error), QStringLiteral("reopen preserved backup"), error);
    expect(restored.purchases().size() == 3, QStringLiteral("failed backup preserves existing rows"));
    expect(db.open(databasePath, &error), QStringLiteral("reopen original database"), error);
    expect(db.purchases().size() == 3, QStringLiteral("backup leaves original rows intact"));
    const QStringList leftovers = QDir(root).entryList(
        {QStringLiteral(".backup.sqlite.backup-*"), QStringLiteral(".backup.sqlite.previous-*")},
        QDir::Files | QDir::Hidden);
    expect(leftovers.isEmpty(), QStringLiteral("backup leaves no temporary files"));
}

void testLegacyDatabaseCompatibility(const QString &root) {
    const QString legacyPath = root + QStringLiteral("/legacy.sqlite");
    const QString connectionName = QStringLiteral("legacy-test-connection");
    {
        QSqlDatabase legacy = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        legacy.setDatabaseName(legacyPath);
        expect(legacy.open(), QStringLiteral("open legacy fixture: %1").arg(legacy.lastError().text()));
        QSqlQuery query(legacy);
        expect(query.exec(QStringLiteral(
            "CREATE TABLE purchases ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "purchase_date TEXT NOT NULL,"
            "site TEXT NOT NULL,"
            "euro_cents INTEGER NOT NULL CHECK(euro_cents >= 0),"
            "sats INTEGER NOT NULL CHECK(sats >= 0),"
            "txid TEXT NOT NULL DEFAULT '',"
            "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP)")),
            QStringLiteral("create legacy schema: %1").arg(query.lastError().text()));
        expect(query.exec(QStringLiteral(
            "INSERT INTO purchases(purchase_date, site, euro_cents, sats, txid) "
            "VALUES('2025-12-31', 'Legacy Exchange', 2500, 500, 'legacy-tx')")),
            QStringLiteral("insert legacy row: %1").arg(query.lastError().text()));
        legacy.close();
    }
    QSqlDatabase::removeDatabase(connectionName);

    Database legacy;
    QString error;
    expect(legacy.open(legacyPath, &error), QStringLiteral("open old database"), error);
    expect(!legacy.hasStoredCurrency(), QStringLiteral("old database is detected as pre-currency"));
    expect(legacy.setCurrency(AppCurrency::Currency::Euro, &error),
           QStringLiteral("mark old database as EUR"), error);
    expect(legacy.purchases(&error).size() == 1,
           QStringLiteral("old database row remains readable"), error);

    const QString backupPath = root + QStringLiteral("/legacy-backup.sqlite");
    expect(legacy.backupTo(backupPath, &error), QStringLiteral("back up old database"), error);
    Database restored;
    expect(restored.open(backupPath, &error), QStringLiteral("reopen old-database backup"), error);
    expect(restored.purchases(&error).size() == 1,
           QStringLiteral("old-database backup preserves data"), error);
}

void testCsvValidation(const QString &root) {
    Database db;
    QString error;
    expect(db.open(root + QStringLiteral("/csv.sqlite"), &error),
           QStringLiteral("open CSV test database: %1").arg(error));
    expect(db.setCurrency(AppCurrency::Currency::Euro, &error),
           QStringLiteral("set CSV test currency: %1").arg(error));

    const QString header = QStringLiteral("Data;Sito / Exchange;Euro spesi;Satoshi;TX / ID transazione\n");
    const QString validPath = root + QStringLiteral("/valid.csv");
    const QString validContents = header
        + QStringLiteral("03/01/2009;A;10,00;100;TxCase\n")
        + QDate::currentDate().toString(QStringLiteral("dd/MM/yyyy"))
        + QStringLiteral(";B;20,00;200;txcase\n");
    expect(writeUtf8(validPath, validContents), QStringLiteral("write valid CSV fixture"));
    const CsvImportResult valid = CsvUtils::importFile(validPath, db);
    expect(valid.validRows.size() == 2, QStringLiteral("accept minimum date, today and case-distinct TXIDs"));
    expect(valid.duplicateRows == 0, QStringLiteral("case-distinct TXIDs are not duplicates"));

    const QString invalidPath = root + QStringLiteral("/invalid.csv");
    const QString invalidContents = header
        + QStringLiteral("02/01/2009;Too Early;10,00;100;early\n")
        + QDate::currentDate().addDays(1).toString(QStringLiteral("dd/MM/yyyy"))
        + QStringLiteral(";Future;10,00;100;future\n")
        + QStringLiteral("03/01/2009;Zero Money;0,00;100;zero-money\n")
        + QStringLiteral("03/01/2009;Zero Sats;10,00;0;zero-sats\n");
    expect(writeUtf8(invalidPath, invalidContents), QStringLiteral("write invalid CSV fixture"));
    const CsvImportResult invalid = CsvUtils::importFile(invalidPath, db);
    expect(invalid.validRows.isEmpty(), QStringLiteral("reject dates outside range and zero values"));
    expect(invalid.errors.size() == 4, QStringLiteral("report every invalid CSV row"));

    const QString noTxidPath = root + QStringLiteral("/no-txid.csv");
    const QString noTxidContents = QStringLiteral("Data;Sito / Exchange;Euro spesi;Satoshi\n")
        + QStringLiteral("03/01/2009;No TXID;10,00;100\n");
    expect(writeUtf8(noTxidPath, noTxidContents), QStringLiteral("write CSV fixture without TXID"));
    const CsvImportResult noTxid = CsvUtils::importFile(noTxidPath, db);
    expect(noTxid.validRows.size() == 1, QStringLiteral("accept CSV without TXID column"));
    expect(noTxid.validRows.first().txid.isEmpty() && !noTxid.validRows.first().txid.isNull(),
           QStringLiteral("normalize missing TXID to a non-null empty string"));
    expect(db.addPurchasesTransaction(noTxid.validRows, &error),
           QStringLiteral("import CSV row without TXID into database: %1").arg(error));

    const QString blankTxidPath = root + QStringLiteral("/blank-txid.csv");
    const QString blankTxidContents = header
        + QStringLiteral("03/01/2009;Blank TXID;10,00;100;\n");
    expect(writeUtf8(blankTxidPath, blankTxidContents), QStringLiteral("write CSV fixture with blank TXID"));
    const CsvImportResult blankTxid = CsvUtils::importFile(blankTxidPath, db);
    expect(blankTxid.validRows.size() == 1, QStringLiteral("accept CSV with blank TXID"));
    expect(db.addPurchasesTransaction(blankTxid.validRows, &error),
           QStringLiteral("import CSV row with blank TXID into database: %1").arg(error));
}


void testCsvExportTotals(const QString &root) {
    int fixture = 0;
    for (bool english : {false, true}) {
        for (auto currency : {AppCurrency::Currency::Euro, AppCurrency::Currency::UsDollar}) {
            for (QChar delimiter : {QChar(';'), QChar(','), QChar('\t')}) {
                const QString name = QStringLiteral("/totals-%1").arg(++fixture);
                Database db;
                QString error;
                expect(db.open(root + name + ".sqlite", &error), "open totals database: " + error);
                expect(db.setCurrency(currency, &error), "set totals currency: " + error);
                const bool usd = currency == AppCurrency::Currency::UsDollar;
                const QStringList headers{
                    english ? "Date" : "Data", english ? "Site / exchange" : "Sito / exchange",
                    usd ? (english ? "USD spent" : "Dollari spesi (USD)")
                        : (english ? "Euro spent" : "Euro spesi"),
                    "BTC on-chain", "Satoshi", english ? "TX / Transaction ID" : "TX / ID transazione"
                };
                auto row = [&](const QStringList &values) {
                    QStringList escaped;
                    for (const QString &v : values) escaped << CsvUtils::csvEscape(v, delimiter);
                    return escaped.join(delimiter) + '\n';
                };
                const QString amount = AppCurrency::plainAmount(12345, currency);
                const QString btc = usd ? "0.00100000" : "0,00100000";
                // Include the UTF-8 BOM and blank line emitted by Export CSV.
                const QString contents = QString(QChar(0xFEFF)) + row(headers)
                    + row({"03/01/2009", "Exchange", amount, btc, "100000", "tx-totals"})
                    + '\n' + row({english ? "TOTALS" : "TOTALI", "", amount, btc, "100000", ""});
                const QString path = root + name + ".csv";
                expect(writeUtf8(path, contents), "write exported CSV fixture");
                const auto imported = CsvUtils::importFile(path, db);
                expect(imported.errors.isEmpty(), "totals summary must not cause errors: " + imported.errors.join("; "));
                expect(imported.validRows.size() == 1 && imported.duplicateRows == 0,
                       "summary must not become a purchase or duplicate");
                expect(db.addPurchasesTransaction(imported.validRows, &error), "save imported purchases: " + error);
                const auto totals = db.totals();
                expect(totals.first == 12345 && totals.second == 100000, "import preserves totals without double counting");
                const auto again = CsvUtils::importFile(path, db);
                expect(again.validRows.isEmpty() && again.duplicateRows == 1 && again.errors.isEmpty(),
                       "reimport reports only the existing purchase as duplicate");
            }
        }
    }

    Database db;
    QString error;
    expect(db.open(root + "/totals-boundaries.sqlite", &error), "open boundary database");
    expect(db.setCurrency(AppCurrency::Currency::Euro, &error), "set boundary currency");
    const QString path = root + "/totals-boundaries.csv";
    // Reordered columns, quoted/trimmed label, real exchange named TOTALS,
    // and malformed purchases that must continue to be reported as errors.
    expect(writeUtf8(path, QStringLiteral(
        "Exchange;EUR;Date;Sats;TXID\n"
        ";10;\" totals \";100;\n"
        "TOTALS;10;03/01/2009;100;real\n"
        "Exchange;10;TOTALI;100;\n"
        ";10;TOTALS;100;nonempty-tx\n"
        ";10;not-a-date;100;\n")), "write totals boundary fixture");
    const auto result = CsvUtils::importFile(path, db);
    expect(result.validRows.size() == 1 && result.errors.size() == 3,
           "skip only recognized summaries; preserve purchases and real errors");
    expect(result.errors.join(" ").contains('6'), "preserve physical CSV error line numbers");
}

void testMonthlyStatistics() {
    QVector<Purchase> purchases;

    Purchase january = purchase(QStringLiteral("monthly-january"), 10000, 100);
    january.date = QDate(2026, 1, 10);
    purchases.push_back(january);

    Purchase marchOne = purchase(QStringLiteral("monthly-march-1"), 20000, 100);
    marchOne.date = QDate(2026, 3, 2);
    purchases.push_back(marchOne);

    Purchase marchTwo = purchase(QStringLiteral("monthly-march-2"), 5000, 100);
    marchTwo.date = QDate(2026, 3, 20);
    purchases.push_back(marchTwo);

    Purchase previousYear = purchase(QStringLiteral("monthly-previous"), 12000, 100);
    previousYear.date = QDate(2025, 12, 15);
    purchases.push_back(previousYear);

    const QDate today(2026, 9, 4);

    const MonthlySummary currentYear = MonthlyStats::calculate(purchases, 2026, today);
    expect(currentYear.months.size() == 9,
           QStringLiteral("current year includes January through the current month"));
    expect(currentYear.months[0].amountCents == 10000,
           QStringLiteral("monthly summary totals January"));
    expect(currentYear.months[1].amountCents == 0,
           QStringLiteral("monthly summary includes zero-spending months"));
    expect(currentYear.months[2].amountCents == 25000,
           QStringLiteral("monthly summary combines purchases in the same month"));
    expect(currentYear.totalCents == 35000,
           QStringLiteral("current-year monthly total"));
    expect(currentYear.averageCents == 3889,
           QStringLiteral("current-year average is rounded to the nearest cent"));

    const MonthlySummary pastYear = MonthlyStats::calculate(purchases, 2025, today);
    expect(pastYear.months.size() == 12,
           QStringLiteral("past year includes all twelve months"));
    expect(pastYear.averageCents == 1000,
           QStringLiteral("past-year average includes zero-spending months"));

    const MonthlySummary allYears = MonthlyStats::calculate(purchases, 0, today);
    expect(allYears.months.size() == 4,
           QStringLiteral("all-years summary spans first through last purchase month"));
    expect(allYears.totalCents == 47000 && allYears.averageCents == 11750,
           QStringLiteral("all-years average includes gaps between first and last purchase"));

    const MonthlySummary empty = MonthlyStats::calculate({}, 0, today);
    expect(empty.months.isEmpty() && empty.averageCents == 0,
           QStringLiteral("empty history has no monthly average"));
}

void testPurchaseSearch() {
    Purchase p = purchase(QStringLiteral("AbC123-Transaction"), 12345, 100000);
    p.date = QDate(2026, 9, 15);
    p.site = QStringLiteral("Kraken Pro");

    using Field = PurchaseSearch::Field;

    expect(PurchaseSearch::matches(p, QString(), Field::All, AppCurrency::Currency::Euro),
           QStringLiteral("empty search matches every purchase"));
    expect(PurchaseSearch::matches(p, QStringLiteral("15/09/2026"), Field::Date, AppCurrency::Currency::Euro),
           QStringLiteral("search full localized date"));
    expect(PurchaseSearch::matches(p, QStringLiteral("09/2026"), Field::Date, AppCurrency::Currency::Euro),
           QStringLiteral("search month and year"));
    expect(PurchaseSearch::matches(p, QStringLiteral("2026-09-15"), Field::Date, AppCurrency::Currency::Euro),
           QStringLiteral("search ISO date"));
    expect(PurchaseSearch::matches(p, QStringLiteral("15.09.2026"), Field::Date, AppCurrency::Currency::Euro),
           QStringLiteral("search date with dots"));
    expect(PurchaseSearch::matches(p, QStringLiteral("krak"), Field::Site, AppCurrency::Currency::Euro),
           QStringLiteral("search exchange case-insensitively"));
    expect(PurchaseSearch::matches(p, QStringLiteral("123,45"), Field::Amount, AppCurrency::Currency::Euro),
           QStringLiteral("search EUR amount with comma"));
    expect(PurchaseSearch::matches(p, QStringLiteral("123.45"), Field::Amount, AppCurrency::Currency::Euro),
           QStringLiteral("search EUR amount with dot"));
    expect(PurchaseSearch::matches(p, QStringLiteral("0,00100000"), Field::Bitcoin, AppCurrency::Currency::Euro),
           QStringLiteral("search BTC value"));
    expect(PurchaseSearch::matches(p, QStringLiteral("100000 sats"), Field::Bitcoin, AppCurrency::Currency::Euro),
           QStringLiteral("search satoshi value"));
    expect(PurchaseSearch::matches(p, QStringLiteral("c123"), Field::Transaction, AppCurrency::Currency::Euro),
           QStringLiteral("search partial TX ID case-insensitively"));
    expect(PurchaseSearch::matches(p, QStringLiteral("Kraken"), Field::All, AppCurrency::Currency::Euro),
           QStringLiteral("all-fields search finds exchange"));
    expect(PurchaseSearch::matches(p, QStringLiteral("123,45"), Field::All, AppCurrency::Currency::Euro),
           QStringLiteral("all-fields search finds amount"));
    expect(PurchaseSearch::matches(p, QStringLiteral("100000 sats"), Field::All, AppCurrency::Currency::Euro),
           QStringLiteral("all-fields search finds satoshi"));
    expect(PurchaseSearch::matches(p, QStringLiteral("transaction"), Field::All, AppCurrency::Currency::Euro),
           QStringLiteral("all-fields search finds partial transaction ID"));
    expect(!PurchaseSearch::matches(p, QStringLiteral("Coinbase"), Field::All, AppCurrency::Currency::Euro),
           QStringLiteral("unrelated search does not match"));

    Purchase usd = p;
    usd.euroCents = 9876;
    expect(PurchaseSearch::matches(usd, QStringLiteral("$98.76"), Field::Amount, AppCurrency::Currency::UsDollar),
           QStringLiteral("search USD amount"));
}
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QTemporaryDir temporaryDir;
    expect(temporaryDir.isValid(), QStringLiteral("create temporary test directory"));
    if (temporaryDir.isValid()) {
        testDatabaseRulesAndBackup(temporaryDir.path());
        testLegacyDatabaseCompatibility(temporaryDir.path());
        testCsvValidation(temporaryDir.path());
        testCsvExportTotals(temporaryDir.path());
        testMonthlyStatistics();
        testPurchaseSearch();
    }
    if (failures == 0)
        QTextStream(stdout) << "All tests passed.\n";
    return failures == 0 ? 0 : 1;
}
