#pragma once

#include "database.h"
#include <QMainWindow>

class QLabel;
class QTableWidget;
class QComboBox;
class PurchasePriceChart;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void addPurchase();
    void editPurchase();
    void deletePurchase();
    void importCsv();
    void exportCsv();
    void exportPdf();
    void backupDatabase();
    void showDatabasePath();
    void changeDatabaseFolder();

private:
    bool initializeDatabase();
    bool openDatabaseAt(const QString &folder, bool remember = true);
    void buildUi();
    void refresh();
    qint64 selectedId() const;
    Purchase selectedPurchase() const;
    QString chooseDatabaseFolder(const QString &title, const QString &initial = QString());

    Database m_db;
    QTableWidget *m_table{};
    QLabel *m_totalEuro{};
    QLabel *m_totalBtc{};
    QLabel *m_totalSats{};
    QLabel *m_averagePrice{};
    QComboBox *m_yearFilter{};
    PurchasePriceChart *m_priceChart{};
    QLabel *m_dbPath{};
};
