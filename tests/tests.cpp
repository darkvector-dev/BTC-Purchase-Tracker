#include "csvutils.h"
#include "database.h"

#include <QCoreApplication>
#include <QDate>
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
    expect(db.open(databasePath, &error), QStringLiteral("open new database: %1").arg(error));
    expect(db.setCurrency(AppCurrency::Currency::Euro, &error),
           QStringLiteral("set database currency: %1").arg(error));

    expect(db.addPurchase(purchase(QStringLiteral("AbC123")), &error),
           QStringLiteral("insert valid purchase: %1").arg(error));
    expect(db.txidExists(QStringLiteral("AbC123")), QStringLiteral("find exact TXID"));
    expect(!db.txidExists(QStringLiteral("abc123")), QStringLiteral("TXID comparison is case-sensitive"));
    expect(db.addPurchase(purchase(QStringLiteral("abc123")), &error),
           QStringLiteral("allow TXID differing only by case: %1").arg(error));

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
    expect(db.backupTo(backupPath, &error), QStringLiteral("create database backup: %1").arg(error));
    expect(!db.backupTo(databasePath, &error), QStringLiteral("reject backup onto active database"));

    expect(db.addPurchase(purchase(QStringLiteral("after-first-backup")), &error),
           QStringLiteral("insert before replacing backup: %1").arg(error));
    expect(db.backupTo(backupPath, &error), QStringLiteral("replace existing backup safely: %1").arg(error));

    Database restored;
    expect(restored.open(backupPath, &error), QStringLiteral("open generated backup: %1").arg(error));
    expect(restored.purchases(&error).size() == 3,
           QStringLiteral("replacement backup contains the latest rows: %1").arg(error));
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
    expect(legacy.open(legacyPath, &error), QStringLiteral("open old database: %1").arg(error));
    expect(!legacy.hasStoredCurrency(), QStringLiteral("old database is detected as pre-currency"));
    expect(legacy.setCurrency(AppCurrency::Currency::Euro, &error),
           QStringLiteral("mark old database as EUR: %1").arg(error));
    expect(legacy.purchases(&error).size() == 1,
           QStringLiteral("old database row remains readable: %1").arg(error));

    const QString backupPath = root + QStringLiteral("/legacy-backup.sqlite");
    expect(legacy.backupTo(backupPath, &error), QStringLiteral("back up old database: %1").arg(error));
    Database restored;
    expect(restored.open(backupPath, &error), QStringLiteral("reopen old-database backup: %1").arg(error));
    expect(restored.purchases(&error).size() == 1,
           QStringLiteral("old-database backup preserves data: %1").arg(error));
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
    }
    if (failures == 0)
        QTextStream(stdout) << "All tests passed.\n";
    return failures == 0 ? 0 : 1;
}
