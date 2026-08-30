#include "database.h"

#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

Database::Database() : m_connectionName(uniqueConnectionName()) {}
Database::~Database() { close(); }

QString Database::uniqueConnectionName() const {
    return QStringLiteral("btc_tracker_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

bool Database::open(const QString &filePath, QString *error) {
    close();
    m_connectionName = uniqueConnectionName();
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(filePath);
    if (!m_db.open()) {
        if (error) *error = m_db.lastError().text();
        return false;
    }
    m_filePath = QFileInfo(filePath).absoluteFilePath();
    if (!ensureSchema(error))
        return false;
    return loadCurrency(error);
}

void Database::close() {
    if (m_db.isValid()) {
        const QString name = m_connectionName;
        m_db.close();
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(name);
    }
    m_filePath.clear();
    m_currency = AppCurrency::Currency::Euro;
    m_hasStoredCurrency = false;
}

bool Database::ensureSchema(QString *error) {
    QSqlQuery q(m_db);
    const char *sql = R"SQL(
        CREATE TABLE IF NOT EXISTS purchases (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            purchase_date TEXT NOT NULL,
            site TEXT NOT NULL,
            euro_cents INTEGER NOT NULL CHECK(euro_cents >= 0),
            sats INTEGER NOT NULL CHECK(sats >= 0),
            txid TEXT NOT NULL DEFAULT '',
            created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
        )
    )SQL";
    if (!q.exec(sql)) {
        if (error) *error = q.lastError().text();
        return false;
    }
    if (!q.exec("CREATE INDEX IF NOT EXISTS idx_purchases_date ON purchases(purchase_date)")) {
        if (error) *error = q.lastError().text();
        return false;
    }
    if (!q.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS app_metadata (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL
        )
    )SQL")) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}

bool Database::loadCurrency(QString *error) {
    m_currency = AppCurrency::Currency::Euro;
    m_hasStoredCurrency = false;

    QSqlQuery q(m_db);
    q.prepare("SELECT value FROM app_metadata WHERE key='currency' LIMIT 1");
    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    if (!q.next())
        return true;

    const QString value = q.value(0).toString().trimmed().toUpper();
    if (value == QStringLiteral("EUR")) {
        m_currency = AppCurrency::Currency::Euro;
    } else if (value == QStringLiteral("USD")) {
        m_currency = AppCurrency::Currency::UsDollar;
    } else {
        if (error) {
            *error = QStringLiteral("Unsupported database currency: %1").arg(value);
        }
        return false;
    }

    m_hasStoredCurrency = true;
    return true;
}

bool Database::setCurrency(AppCurrency::Currency currency, QString *error) {
    // La valuta è una proprietà immutabile del database: una volta salvata
    // può soltanto essere riletta, mai sostituita con un'altra valuta.
    if (m_hasStoredCurrency) {
        if (m_currency == currency)
            return true;
        if (error)
            *error = QStringLiteral("Database currency is already configured as %1.")
                         .arg(AppCurrency::code(m_currency));
        return false;
    }

    QSqlQuery q(m_db);
    q.prepare("INSERT INTO app_metadata(key, value) VALUES('currency', ?)");
    q.addBindValue(AppCurrency::code(currency));
    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    m_currency = currency;
    m_hasStoredCurrency = true;
    return true;
}

QVector<Purchase> Database::purchases(QString *error) const {
    QVector<Purchase> out;
    QSqlQuery q(m_db);
    if (!q.exec("SELECT id, purchase_date, site, euro_cents, sats, txid FROM purchases ORDER BY purchase_date DESC, id DESC")) {
        if (error) *error = q.lastError().text();
        return out;
    }
    while (q.next()) {
        Purchase p;
        p.id = q.value(0).toLongLong();
        p.date = QDate::fromString(q.value(1).toString(), Qt::ISODate);
        p.site = q.value(2).toString();
        p.euroCents = q.value(3).toLongLong();
        p.sats = q.value(4).toLongLong();
        p.txid = q.value(5).toString();
        out.push_back(p);
    }
    return out;
}

bool Database::addPurchase(const Purchase &p, QString *error) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO purchases (purchase_date, site, euro_cents, sats, txid) VALUES (?, ?, ?, ?, ?)");
    q.addBindValue(p.date.toString(Qt::ISODate));
    q.addBindValue(p.site);
    q.addBindValue(p.euroCents);
    q.addBindValue(p.sats);
    q.addBindValue(p.txid);
    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}

bool Database::updatePurchase(const Purchase &p, QString *error) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE purchases SET purchase_date=?, site=?, euro_cents=?, sats=?, txid=? WHERE id=?");
    q.addBindValue(p.date.toString(Qt::ISODate));
    q.addBindValue(p.site);
    q.addBindValue(p.euroCents);
    q.addBindValue(p.sats);
    q.addBindValue(p.txid);
    q.addBindValue(p.id);
    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}

bool Database::deletePurchase(qint64 id, QString *error) {
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM purchases WHERE id=?");
    q.addBindValue(id);
    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}

bool Database::txidExists(const QString &txid, qint64 excludeId) const {
    if (txid.trimmed().isEmpty()) return false;
    QSqlQuery q(m_db);
    if (excludeId >= 0) {
        q.prepare("SELECT 1 FROM purchases WHERE txid=? AND id<>? LIMIT 1");
        q.addBindValue(txid.trimmed());
        q.addBindValue(excludeId);
    } else {
        q.prepare("SELECT 1 FROM purchases WHERE txid=? LIMIT 1");
        q.addBindValue(txid.trimmed());
    }
    return q.exec() && q.next();
}

bool Database::addPurchasesTransaction(const QVector<Purchase> &rows, QString *error) {
    if (!m_db.transaction()) {
        if (error) *error = m_db.lastError().text();
        return false;
    }
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO purchases (purchase_date, site, euro_cents, sats, txid) VALUES (?, ?, ?, ?, ?)");
    for (const auto &p : rows) {
        q.bindValue(0, p.date.toString(Qt::ISODate));
        q.bindValue(1, p.site);
        q.bindValue(2, p.euroCents);
        q.bindValue(3, p.sats);
        q.bindValue(4, p.txid);
        if (!q.exec()) {
            m_db.rollback();
            if (error) *error = q.lastError().text();
            return false;
        }
        q.finish();
    }
    if (!m_db.commit()) {
        if (error) *error = m_db.lastError().text();
        return false;
    }
    return true;
}

QPair<qint64, qint64> Database::totals(QString *error) const {
    QSqlQuery q(m_db);
    if (!q.exec("SELECT COALESCE(SUM(euro_cents),0), COALESCE(SUM(sats),0) FROM purchases")) {
        if (error) *error = q.lastError().text();
        return {0, 0};
    }
    if (q.next()) return {q.value(0).toLongLong(), q.value(1).toLongLong()};
    return {0, 0};
}
