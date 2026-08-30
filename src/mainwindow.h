#pragma once

#include "database.h"
#include <QMainWindow>

class QLabel;
class QTableWidget;
class QComboBox;
class QPushButton;
class QMenu;
class QAction;
class PurchasePriceChart;
class QCloseEvent;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

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
    void showAbout();

private:
    bool initializeDatabase();
    bool openDatabaseAt(const QString &folder, bool remember = true);
    bool chooseInitialCurrency(AppCurrency::Currency *currency);
    void buildUi();
    void applyLanguage();
    void changeLanguage(bool english);
    void refresh();
    void restoreUiState();
    void saveUiState() const;
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

    QLabel *m_cardEuroCaption{};
    QLabel *m_cardBtcCaption{};
    QLabel *m_cardSatsCaption{};
    QLabel *m_cardAverageCaption{};
    QLabel *m_filterLabel{};
    QLabel *m_chartTitle{};

    QPushButton *m_addButton{};
    QPushButton *m_editButton{};
    QPushButton *m_deleteButton{};
    QPushButton *m_importButton{};
    QPushButton *m_exportCsvButton{};
    QPushButton *m_exportPdfButton{};
    QPushButton *m_backupButton{};

    QMenu *m_databaseMenu{};
    QMenu *m_settingsMenu{};
    QMenu *m_languageMenu{};
    QMenu *m_infoMenu{};

    QAction *m_showPathAction{};
    QAction *m_changeFolderAction{};
    QAction *m_italianAction{};
    QAction *m_englishAction{};
    QAction *m_aboutAction{};
};
