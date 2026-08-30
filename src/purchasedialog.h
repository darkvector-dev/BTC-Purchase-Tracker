#pragma once

#include "currency.h"
#include "database.h"
#include <QDialog>

class QDateEdit;
class QLineEdit;

class PurchaseDialog : public QDialog {
    Q_OBJECT
public:
    explicit PurchaseDialog(
        QWidget *parent,
        AppCurrency::Currency currency,
        const Purchase *initial = nullptr
    );
    Purchase purchase() const { return m_purchase; }

private slots:
    void syncFromBtc();
    void syncFromSats();
    void validateAndAccept();

private:
    QDateEdit *m_date{};
    QLineEdit *m_site{};
    QLineEdit *m_amount{};
    QLineEdit *m_btc{};
    QLineEdit *m_sats{};
    QLineEdit *m_txid{};
    AppCurrency::Currency m_currency{AppCurrency::Currency::Euro};
    Purchase m_purchase;
};
