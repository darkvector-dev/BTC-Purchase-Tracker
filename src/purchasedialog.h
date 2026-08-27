#pragma once

#include "database.h"
#include <QDialog>

class QDateEdit;
class QLineEdit;

class PurchaseDialog : public QDialog {
    Q_OBJECT
public:
    explicit PurchaseDialog(QWidget *parent = nullptr, const Purchase *initial = nullptr);
    Purchase purchase() const { return m_purchase; }

private slots:
    void syncFromBtc();
    void syncFromSats();
    void validateAndAccept();

private:
    QDateEdit *m_date{};
    QLineEdit *m_site{};
    QLineEdit *m_euro{};
    QLineEdit *m_btc{};
    QLineEdit *m_sats{};
    QLineEdit *m_txid{};
    Purchase m_purchase;
};
