#pragma once

#include <QDate>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

struct Purchase {
    qint64 id = -1;
    QDate date;
    QString site;
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

    QVector<Purchase> purchases(QString *error = nullptr) const;
    bool addPurchase(const Purchase &p, QString *error = nullptr);
    bool updatePurchase(const Purchase &p, QString *error = nullptr);
    bool deletePurchase(qint64 id, QString *error = nullptr);
    bool txidExists(const QString &txid, qint64 excludeId = -1) const;
    bool addPurchasesTransaction(const QVector<Purchase> &rows, QString *error = nullptr);
    QPair<qint64, qint64> totals(QString *error = nullptr) const; // euro cents, sats

private:
    bool ensureSchema(QString *error);
    QString uniqueConnectionName() const;

    QSqlDatabase m_db;
    QString m_connectionName;
    QString m_filePath;
};
