#include "purchasedialog.h"

#include <QApplication>
#include <QDateEdit>
#include <QMessageBox>
#include <QTextStream>
#include <QTimer>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    int failures = 0;
    const QDate today = QDate::currentDate();
    for (const QDate &date : {QDate(2009, 1, 2), QDate(2009, 1, 3), today, today.addDays(1)}) {
        Purchase initial;
        initial.date = date;
        initial.site = QStringLiteral("Test Exchange");
        initial.euroCents = 12345;
        initial.sats = 100000;
        initial.txid = QStringLiteral("test-date");
        PurchaseDialog dialog(nullptr, AppCurrency::Currency::Euro, &initial);
        const bool allowed = date >= QDate(2009, 1, 3) && date <= today;
        bool warned = false;
        // Close the real modal warning so the validation slot can return.
        QTimer closer;
        QObject::connect(&closer, &QTimer::timeout, &dialog, [&] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *message = qobject_cast<QMessageBox *>(widget)) {
                    warned = true;
                    message->accept();
                }
            }
        });
        closer.start(10);
        auto *editor = dialog.findChild<QDateEdit *>();
        if (!editor || editor->date() != date) {
            QTextStream(stderr) << "FAIL: existing date was silently changed\n";
            ++failures;
        }
        const bool invoked = QMetaObject::invokeMethod(&dialog, "validateAndAccept", Qt::DirectConnection);
        closer.stop();
        if (!invoked || (dialog.result() == QDialog::Accepted) != allowed || warned == allowed) {
            QTextStream(stderr) << "FAIL: manual date validation for " << date.toString(Qt::ISODate) << '\n';
            ++failures;
        }
        if (dialog.purchase().date != date) {
            QTextStream(stderr) << "FAIL: validation changed the stored date\n";
            ++failures;
        }
    }
    if (!failures) QTextStream(stdout) << "Purchase date dialog tests passed.\n";
    return failures ? 1 : 0;
}
