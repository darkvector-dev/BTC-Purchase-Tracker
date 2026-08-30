#include "mainwindow.h"
#include "language.h"

#include <QApplication>
#include <QIcon>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("BTCPurchaseTracker");
    QApplication::setApplicationName("BTCPurchaseTracker");
    QApplication::setApplicationVersion("1.0.0");

    AppLanguage::load();

    // L'icona è incorporata nell'eseguibile tramite Qt Resource System,
    // quindi funziona anche dentro l'AppImage e indipendentemente
    // dalla cartella da cui viene avviata l'app.
    app.setWindowIcon(QIcon(":/icons/btc_purchase_tracker_icon.png"));

    MainWindow w;
    w.show();
    return app.exec();
}
