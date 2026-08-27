#include "purchasedialog.h"
#include "csvutils.h"

#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

PurchaseDialog::PurchaseDialog(QWidget *parent, const Purchase *initial) : QDialog(parent) {
    setWindowTitle(initial ? "Modifica acquisto" : "Nuovo acquisto");
    setMinimumWidth(520);

    m_date = new QDateEdit(QDate::currentDate(), this);
    m_date->setCalendarPopup(true);
    m_date->setDisplayFormat("dd/MM/yyyy");
    m_site = new QLineEdit(this);
    m_euro = new QLineEdit(this);
    m_euro->setPlaceholderText("es. 250,00");
    m_btc = new QLineEdit(this);
    m_btc->setPlaceholderText("es. 0,00234567");
    m_sats = new QLineEdit(this);
    m_sats->setPlaceholderText("es. 78489,696901");
    m_txid = new QLineEdit(this);

    if (initial) {
        m_purchase = *initial;
        m_date->setDate(initial->date);
        m_site->setText(initial->site);
        m_euro->setText(QString::number(initial->euroCents / 100.0, 'f', 2).replace('.', ','));
        m_btc->setText(CsvUtils::satsToBtc(initial->sats));
        m_sats->setText(QString::number(initial->sats));
        m_txid->setText(initial->txid);
    }

    auto *form = new QFormLayout;
    form->addRow("Data", m_date);
    form->addRow("Sito / exchange", m_site);
    form->addRow("Euro spesi", m_euro);
    form->addRow("BTC on-chain", m_btc);
    form->addRow("Satoshi", m_sats);
    form->addRow("TX / ID transazione", m_txid);

    auto *note = new QLabel("BTC e satoshi sono sincronizzati. Puoi incollare anche sats frazionari: vengono arrotondati al satoshi intero più vicino per il valore on-chain.", this);
    note->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText("Salva");
    buttons->button(QDialogButtonBox::Cancel)->setText("Annulla");

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(note);
    layout->addWidget(buttons);

    // Aggiornamento immediato del campo opposto mentre l'utente digita.
    // textEdited viene emesso solo per modifiche fatte dall'utente:
    // i setText() qui sotto non provocano quindi ricorsione.
    connect(m_btc, &QLineEdit::textEdited, this, [this](const QString &text) {
        qint64 sats;
        if (CsvUtils::parseBtcToSats(text, &sats)) {
            m_sats->setText(QString::number(sats));
        }
    });

    connect(m_sats, &QLineEdit::textEdited, this, [this](const QString &text) {
        qint64 sats;
        if (CsvUtils::parseSats(text, &sats)) {
            m_btc->setText(CsvUtils::satsToBtc(sats));
        }
    });

    // Quando si esce dal campo, normalizza anche il valore appena inserito.
    connect(m_btc, &QLineEdit::editingFinished, this, &PurchaseDialog::syncFromBtc);
    connect(m_sats, &QLineEdit::editingFinished, this, &PurchaseDialog::syncFromSats);
    connect(buttons, &QDialogButtonBox::accepted, this, &PurchaseDialog::validateAndAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void PurchaseDialog::syncFromBtc() {
    qint64 sats;
    if (CsvUtils::parseBtcToSats(m_btc->text(), &sats)) {
        m_sats->setText(QString::number(sats));
        m_btc->setText(CsvUtils::satsToBtc(sats));
    }
}

void PurchaseDialog::syncFromSats() {
    qint64 sats;
    if (CsvUtils::parseSats(m_sats->text(), &sats)) {
        m_sats->setText(QString::number(sats));
        m_btc->setText(CsvUtils::satsToBtc(sats));
    }
}

void PurchaseDialog::validateAndAccept() {
    Purchase p = m_purchase;
    p.date = m_date->date();
    p.site = m_site->text().trimmed();
    p.txid = m_txid->text().trimmed();
    if (p.site.isEmpty()) {
        QMessageBox::warning(this, "Dato mancante", "Inserisci il sito o exchange.");
        return;
    }
    if (!CsvUtils::parseEuroCents(m_euro->text(), &p.euroCents)) {
        QMessageBox::warning(this, "Dato non valido", "L'importo in euro non è valido.");
        return;
    }
    qint64 b=-1, s=-1;
    const bool bOk = CsvUtils::parseBtcToSats(m_btc->text(), &b);
    const bool sOk = CsvUtils::parseSats(m_sats->text(), &s);

    if (!m_sats->text().trimmed().isEmpty() && !sOk) {
        QMessageBox::warning(
            this,
            "Satoshi non validi",
            "Formato satoshi non riconosciuto. Puoi usare, per esempio, "
            "78489,696901 oppure 3,622.323."
        );
        return;
    }

    if (!bOk && !sOk) {
        QMessageBox::warning(this, "Dato non valido", "Inserisci un valore BTC o satoshi valido.");
        return;
    }
    if (bOk && sOk && b != s) {
        QMessageBox::warning(this, "Valori incoerenti", "BTC e satoshi non corrispondono tra loro.");
        return;
    }
    p.sats = sOk ? s : b;
    m_purchase = p;
    accept();
}
