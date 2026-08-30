#pragma once

#include "currency.h"

#include <QDate>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

struct Purchase {
    qint64 id = -1;
    QDate date;
    QString site;
    // Nome storico mantenuto per compatibilità col database esistente.
    // Il valore rappresenta i centesimi della valuta scelta dal database.
    qint64 euroCents = 0;
    qint64 sats = 0;
    QString txid;
};

class Database {
public:
    Database();
    ~Database();

    bool open(const QString &filePath, QString *error = nullptr);
    void close();
    QString filePath() const { return m_filePath; }

    bool hasStoredCurrency() const { return m_hasStoredCurrency; }
    AppCurrency::Currency currency() const { return m_currency; }
    bool setCurrency(AppCurrency::Currency currency, QString *error = nullptr);

    QVector<Purchase> purchases(QString *error = nullptr) const;
    bool addPurchase(const Purchase &p, QString *error = nullptr);
    bool updatePurchase(const Purchase &p, QString *error = nullptr);
    bool deletePurchase(qint64 id, QString *error = nullptr);
    bool txidExists(const QString &txid, qint64 excludeId = -1) const;
    bool addPurchasesTransaction(const QVector<Purchase> &rows, QString *error = nullptr);
    QPair<qint64, qint64> totals(QString *error = nullptr) const; // valuta in centesimi, sats

private:
    bool ensureSchema(QString *error);
    bool loadCurrency(QString *error);
    QString uniqueConnectionName() const;

    QSqlDatabase m_db;
    QString m_connectionName;
    QString m_filePath;
    AppCurrency::Currency m_currency{AppCurrency::Currency::Euro};
    bool m_hasStoredCurrency{false};
};
