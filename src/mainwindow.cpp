#include "mainwindow.h"
#include "csvutils.h"
#include "purchasedialog.h"

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPdfWriter>
#include <QPageLayout>
#include <QPageSize>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPushButton>
#include <QSaveFile>
#include <QSet>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QSettings>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTextDocument>
#include <QTextStream>
#include <QToolTip>
#include <QStringConverter>
#include <QUrl>
#include <QVBoxLayout>
#include <QVector>

#include <algorithm>
#include <limits>


class PurchasePriceChart : public QWidget {
public:
    explicit PurchasePriceChart(QWidget *parent = nullptr)
        : QWidget(parent) {
        setMinimumHeight(190);
        setMaximumHeight(230);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setMouseTracking(true);
    }

    void setData(QVector<QPair<QDate, double>> points, double averagePrice) {
        std::sort(points.begin(), points.end(),
                  [](const auto &a, const auto &b) {
                      return a.first < b.first;
                  });
        m_points = std::move(points);
        m_averagePrice = averagePrice;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QColor textColor = palette().color(QPalette::Text);
        const QColor mutedColor = palette().color(QPalette::PlaceholderText);
        const QColor gridColor = palette().color(QPalette::Mid);
        const QColor lineColor = palette().color(QPalette::Highlight);
        const QColor backgroundColor = palette().color(QPalette::Base);

        QRectF card = rect().adjusted(1, 1, -1, -1);
        painter.setPen(QPen(gridColor, 1));
        painter.setBrush(backgroundColor);
        painter.drawRoundedRect(card, 8, 8);

        if (m_points.isEmpty()) {
            painter.setPen(mutedColor);
            painter.drawText(
                card.adjusted(20, 20, -20, -20),
                Qt::AlignCenter,
                "Nessun acquisto da visualizzare"
            );
            return;
        }

        const QRectF plot = card.adjusted(72, 18, -20, -38);
        if (plot.width() <= 1 || plot.height() <= 1)
            return;

        double minPrice = m_points.first().second;
        double maxPrice = m_points.first().second;

        for (const auto &point : m_points) {
            minPrice = qMin(minPrice, point.second);
            maxPrice = qMax(maxPrice, point.second);
        }

        if (m_averagePrice > 0.0) {
            minPrice = qMin(minPrice, m_averagePrice);
            maxPrice = qMax(maxPrice, m_averagePrice);
        }

        if (qFuzzyCompare(minPrice + 1.0, maxPrice + 1.0)) {
            const double pad = qMax(100.0, minPrice * 0.05);
            minPrice -= pad;
            maxPrice += pad;
        } else {
            const double pad = (maxPrice - minPrice) * 0.10;
            minPrice = qMax(0.0, minPrice - pad);
            maxPrice += pad;
        }

        QLocale it(QLocale::Italian, QLocale::Italy);

        // Griglia e scala prezzi.
        painter.setFont(QFont(painter.font().family(), qMax(8, painter.font().pointSize() - 1)));
        for (int i = 0; i <= 4; ++i) {
            const double ratio = static_cast<double>(i) / 4.0;
            const double y = plot.bottom() - ratio * plot.height();
            const double price = minPrice + ratio * (maxPrice - minPrice);

            QColor grid = gridColor;
            grid.setAlpha(90);
            painter.setPen(QPen(grid, 1));
            painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));

            painter.setPen(mutedColor);
            const QString label = it.toString(price, 'f', 0) + " €";
            painter.drawText(
                QRectF(card.left() + 6, y - 9, 60, 18),
                Qt::AlignRight | Qt::AlignVCenter,
                label
            );
        }

        const qint64 firstDay = m_points.first().first.toJulianDay();
        const qint64 lastDay = m_points.last().first.toJulianDay();
        const bool sameDay = firstDay == lastDay;

        auto xForIndex = [&](int index) -> double {
            if (m_points.size() == 1)
                return plot.center().x();

            if (sameDay) {
                return plot.left()
                    + (static_cast<double>(index) / (m_points.size() - 1))
                    * plot.width();
            }

            const qint64 day = m_points[index].first.toJulianDay();
            return plot.left()
                + (static_cast<double>(day - firstDay)
                   / static_cast<double>(lastDay - firstDay))
                * plot.width();
        };

        auto yForPrice = [&](double price) -> double {
            return plot.bottom()
                - ((price - minPrice) / (maxPrice - minPrice))
                * plot.height();
        };

        // Linea del prezzo medio.
        if (m_averagePrice > 0.0) {
            const double avgY = yForPrice(m_averagePrice);
            QColor avgColor = lineColor;
            avgColor.setAlpha(150);

            QPen avgPen(avgColor, 1.5, Qt::DashLine);
            painter.setPen(avgPen);
            painter.drawLine(QPointF(plot.left(), avgY), QPointF(plot.right(), avgY));

            painter.setPen(textColor);
            painter.drawText(
                QRectF(plot.left() + 6, avgY - 20, plot.width() - 12, 18),
                Qt::AlignRight | Qt::AlignVCenter,
                "Media " + it.toString(m_averagePrice, 'f', 0) + " €/BTC"
            );
        }

        // Curva cronologica degli acquisti.
        QPainterPath path;
        for (int i = 0; i < m_points.size(); ++i) {
            const QPointF point(xForIndex(i), yForPrice(m_points[i].second));
            if (i == 0)
                path.moveTo(point);
            else
                path.lineTo(point);
        }

        painter.setPen(QPen(lineColor, 2.2));
        if (m_points.size() > 1)
            painter.drawPath(path);

        painter.setBrush(lineColor);
        painter.setPen(QPen(backgroundColor, 1.5));
        for (int i = 0; i < m_points.size(); ++i) {
            const QPointF point(xForIndex(i), yForPrice(m_points[i].second));
            painter.drawEllipse(point, 4.0, 4.0);
        }

        // Indicatore interattivo: linea verticale rossa che segue il mouse.
        // Data e prezzo mostrati sono quelli dell'acquisto reale più vicino
        // alla posizione orizzontale del cursore.
        if (m_hoverActive && m_hoverX >= plot.left() && m_hoverX <= plot.right()) {
            const double hoverX = qBound(plot.left(), m_hoverX, plot.right());

            int nearestIndex = 0;
            double nearestDistance = std::numeric_limits<double>::max();

            for (int i = 0; i < m_points.size(); ++i) {
                const double distance = qAbs(xForIndex(i) - hoverX);
                if (distance < nearestDistance) {
                    nearestDistance = distance;
                    nearestIndex = i;
                }
            }

            const QPointF nearestPoint(
                xForIndex(nearestIndex),
                yForPrice(m_points[nearestIndex].second)
            );

            QColor hoverColor = Qt::red;
            hoverColor.setAlpha(210);

            painter.setPen(QPen(hoverColor, 1.2));
            painter.drawLine(
                QPointF(hoverX, plot.top()),
                QPointF(hoverX, plot.bottom())
            );

            painter.setBrush(hoverColor);
            painter.setPen(QPen(backgroundColor, 2.0));
            painter.drawEllipse(nearestPoint, 6.0, 6.0);

            const QString info =
                m_points[nearestIndex].first.toString("dd/MM/yyyy")
                + "   "
                + it.toString(m_points[nearestIndex].second, 'f', 2)
                + " €/BTC";

            const QFontMetrics fm(painter.font());
            const int textWidth = fm.horizontalAdvance(info);
            const int boxWidth = textWidth + 18;
            const int boxHeight = fm.height() + 12;

            double boxX = hoverX + 10;
            if (boxX + boxWidth > plot.right())
                boxX = hoverX - boxWidth - 10;

            boxX = qBound(plot.left(), boxX, plot.right() - boxWidth);
            const double boxY = plot.top() + 8;

            QRectF infoBox(boxX, boxY, boxWidth, boxHeight);

            painter.setPen(QPen(gridColor, 1));
            painter.setBrush(palette().color(QPalette::ToolTipBase));
            painter.drawRoundedRect(infoBox, 5, 5);

            painter.setPen(palette().color(QPalette::ToolTipText));
            painter.drawText(
                infoBox.adjusted(9, 5, -9, -5),
                Qt::AlignCenter,
                info
            );
        }

        // Date: prima, centrale (se utile), ultima.
        painter.setPen(mutedColor);
        const QString firstLabel = m_points.first().first.toString("dd/MM/yy");
        const QString lastLabel = m_points.last().first.toString("dd/MM/yy");

        painter.drawText(
            QRectF(plot.left(), plot.bottom() + 8, 90, 20),
            Qt::AlignLeft | Qt::AlignVCenter,
            firstLabel
        );

        if (m_points.size() >= 3) {
            const int middle = m_points.size() / 2;
            painter.drawText(
                QRectF(plot.center().x() - 50, plot.bottom() + 8, 100, 20),
                Qt::AlignCenter,
                m_points[middle].first.toString("dd/MM/yy")
            );
        }

        if (m_points.size() > 1) {
            painter.drawText(
                QRectF(plot.right() - 90, plot.bottom() + 8, 90, 20),
                Qt::AlignRight | Qt::AlignVCenter,
                lastLabel
            );
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        const QRectF card = rect().adjusted(1, 1, -1, -1);
        const QRectF plot = card.adjusted(72, 18, -20, -38);

        m_hoverX = event->position().x();
        m_hoverActive =
            !m_points.isEmpty()
            && event->position().y() >= plot.top()
            && event->position().y() <= plot.bottom()
            && m_hoverX >= plot.left()
            && m_hoverX <= plot.right();

        update();
        QWidget::mouseMoveEvent(event);
    }

    void leaveEvent(QEvent *event) override {
        m_hoverActive = false;
        update();
        QWidget::leaveEvent(event);
    }

private:
    QVector<QPair<QDate, double>> m_points;
    double m_averagePrice{};
    double m_hoverX{};
    bool m_hoverActive{false};
};

namespace {
constexpr auto kOrg = "BTCPurchaseTracker";
constexpr auto kApp = "BTCPurchaseTracker";
constexpr auto kDbKey = "databaseFolder";
constexpr auto kWindowGeometryKey = "ui/windowGeometry";
constexpr auto kTableHeaderStateKey = "ui/tableHeaderState";
constexpr auto kDbName = "btc-purchase-tracker.sqlite";
constexpr auto kAutoCsvName = "btc_purchase_tracker_autobackup.csv";

QString htmlEscape(QString s) { return s.toHtmlEscaped(); }

bool writeAutomaticCsvBackup(const Database &db, QString *error = nullptr) {
    QString queryError;
    const auto rows = db.purchases(&queryError);
    if (!queryError.isEmpty()) {
        if (error) *error = queryError;
        return false;
    }

    const auto [euro, sats] = db.totals(&queryError);
    if (!queryError.isEmpty()) {
        if (error) *error = queryError;
        return false;
    }

    const QString path =
        QFileInfo(db.filePath()).absoluteDir().filePath(kAutoCsvName);

    // QSaveFile scrive prima su un file temporaneo e sostituisce il CSV
    // precedente solo dopo una scrittura completata con successo.
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) *error = file.errorString();
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << QChar(0xFEFF);

    const QChar delimiter = ';';
    out << "Data;Sito / exchange;Euro spesi;BTC on-chain;Satoshi;TX / ID transazione\n";

    for (const auto &p : rows) {
        out << p.date.toString("dd/MM/yyyy") << delimiter
            << CsvUtils::csvEscape(p.site, delimiter) << delimiter
            << QString::number(p.euroCents / 100.0, 'f', 2).replace('.', ',') << delimiter
            << CsvUtils::satsToBtc(p.sats).replace('.', ',') << delimiter
            << p.sats << delimiter
            << CsvUtils::csvEscape(p.txid, delimiter) << "\n";
    }

    out << "\nTOTALI;;"
        << QString::number(euro / 100.0, 'f', 2).replace('.', ',') << delimiter
        << CsvUtils::satsToBtc(sats).replace('.', ',') << delimiter
        << sats << delimiter << "\n";

    if (!file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }

    return true;
}

class ClickableValueLabel : public QLabel {
public:
    explicit ClickableValueLabel(QWidget *parent = nullptr)
        : QLabel(parent) {
        setCursor(Qt::PointingHandCursor);
        setToolTip("Clicca per copiare");
    }

protected:
    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            const QString value = property("clipboardValue").toString();
            if (!value.isEmpty()) {
                QApplication::clipboard()->setText(value);

                if (auto *mainWindow = qobject_cast<QMainWindow *>(window())) {
                    mainWindow->statusBar()->showMessage(
                        "✓ Copiato negli appunti",
                        1500
                    );
                }
            }
        }
        QLabel::mouseReleaseEvent(event);
    }
};

class SortableNumberItem : public QTableWidgetItem {
public:
    SortableNumberItem(const QString &text, qint64 sortValue)
        : QTableWidgetItem(text), m_sortValue(sortValue) {}

    bool operator<(const QTableWidgetItem &other) const override {
        const auto *otherItem = dynamic_cast<const SortableNumberItem *>(&other);
        if (otherItem) return m_sortValue < otherItem->m_sortValue;
        return QTableWidgetItem::operator<(other);
    }

private:
    qint64 m_sortValue;
};
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("BTC Purchase Tracker");
    resize(1220, 880);
    buildUi();
    restoreUiState();

    if (!initializeDatabase()) {
        QMetaObject::invokeMethod(qApp, &QApplication::quit, Qt::QueuedConnection);
        return;
    }

    refresh();
}

void MainWindow::restoreUiState() {
    QSettings settings(kOrg, kApp);

    const QByteArray geometry =
        settings.value(kWindowGeometryKey).toByteArray();
    if (!geometry.isEmpty())
        restoreGeometry(geometry);

    if (m_table) {
        const QByteArray headerState =
            settings.value(kTableHeaderStateKey).toByteArray();

        if (!headerState.isEmpty())
            m_table->horizontalHeader()->restoreState(headerState);
    }
}

void MainWindow::saveUiState() const {
    QSettings settings(kOrg, kApp);
    settings.setValue(kWindowGeometryKey, saveGeometry());

    if (m_table) {
        settings.setValue(
            kTableHeaderStateKey,
            m_table->horizontalHeader()->saveState()
        );
    }

    settings.sync();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    saveUiState();
    QMainWindow::closeEvent(event);
}

QString MainWindow::chooseDatabaseFolder(const QString &title, const QString &initial) {
    QString start = initial;
    if (start.isEmpty()) start = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    return QFileDialog::getExistingDirectory(this, title, start, QFileDialog::ShowDirsOnly);
}

bool MainWindow::initializeDatabase() {
    QSettings settings(kOrg, kApp);
    QString folder = settings.value(kDbKey).toString();
    if (folder.isEmpty() || !QDir(folder).exists()) {
        QMessageBox::information(this, "Prima configurazione",
            "Scegli la cartella in cui vuoi conservare il database degli acquisti.\n\n"
            "Il file rimarrà in quella posizione anche aggiornando o spostando l'applicazione.");
        folder = chooseDatabaseFolder("Scegli la cartella del database");
        if (folder.isEmpty()) return false;
    }
    return openDatabaseAt(folder, true);
}

bool MainWindow::openDatabaseAt(const QString &folder, bool remember) {
    QDir dir(folder);
    if (!dir.exists() && !dir.mkpath(".")) {
        QMessageBox::critical(this, "Errore", "Impossibile creare o accedere alla cartella scelta.");
        return false;
    }
    const QString path = dir.filePath(kDbName);
    QString error;
    if (!m_db.open(path, &error)) {
        QMessageBox::critical(this, "Errore database", error);
        return false;
    }
    if (remember) {
        QSettings settings(kOrg, kApp);
        settings.setValue(kDbKey, QFileInfo(folder).absoluteFilePath());
    }

    QString csvError;
    if (!writeAutomaticCsvBackup(m_db, &csvError)) {
        QMessageBox::warning(
            this,
            "Backup CSV automatico",
            "Il database è stato aperto correttamente, ma non è stato possibile "
            "aggiornare il CSV automatico:\n\n" + csvError
        );
    }

    if (m_dbPath) m_dbPath->setText(path);
    return true;
}

void MainWindow::buildUi() {
    auto *central = new QWidget(this);
    auto *outer = new QVBoxLayout(central);
    outer->setContentsMargins(18,18,18,18);
    outer->setSpacing(12);

    auto *titleRow = new QHBoxLayout;
    auto *title = new QLabel("₿  BTC Purchase Tracker", this);
    QFont tf = title->font(); tf.setPointSize(tf.pointSize()+6); tf.setBold(true); title->setFont(tf);
    auto *add = new QPushButton("+ Nuovo acquisto", this);
    titleRow->addWidget(title);
    titleRow->addStretch();
    titleRow->addWidget(add);
    outer->addLayout(titleRow);

    auto *cards = new QHBoxLayout;
    auto makeCard = [&](const QString &caption, QLabel **value) {
        auto *box = new QFrame(this);
        box->setFrameShape(QFrame::StyledPanel);
        auto *l = new QVBoxLayout(box);
        auto *c = new QLabel(caption, box);
        QFont cf=c->font(); cf.setBold(true); c->setFont(cf);
        *value = new ClickableValueLabel(box);
        (*value)->setText("—");
        QFont vf=(*value)->font(); vf.setPointSize(vf.pointSize()+4); vf.setBold(true); (*value)->setFont(vf);
        l->addWidget(c); l->addWidget(*value);
        cards->addWidget(box);
    };
    makeCard("EURO SPESI", &m_totalEuro);
    makeCard("BTC ACQUISTATI", &m_totalBtc);
    makeCard("SATOSHI", &m_totalSats);
    makeCard("PREZZO MEDIO €/BTC", &m_averagePrice);
    outer->addLayout(cards);

    auto *filterRow = new QHBoxLayout;
    auto *filterLabel = new QLabel("Anno:", this);
    QFont ff = filterLabel->font();
    ff.setBold(true);
    filterLabel->setFont(ff);

    m_yearFilter = new QComboBox(this);
    m_yearFilter->setMinimumWidth(150);
    m_yearFilter->setToolTip("Filtra la tabella e i totali per anno");

    filterRow->addWidget(filterLabel);
    filterRow->addWidget(m_yearFilter);
    filterRow->addStretch();
    outer->addLayout(filterRow);

    auto *chartBox = new QFrame(this);
    chartBox->setFrameShape(QFrame::NoFrame);
    auto *chartLayout = new QVBoxLayout(chartBox);
    chartLayout->setContentsMargins(0, 0, 0, 0);
    chartLayout->setSpacing(5);

    auto *chartTitle = new QLabel("ANDAMENTO PREZZO DI ACQUISTO", chartBox);
    QFont chartTitleFont = chartTitle->font();
    chartTitleFont.setBold(true);
    chartTitle->setFont(chartTitleFont);

    m_priceChart = new PurchasePriceChart(chartBox);
    chartLayout->addWidget(chartTitle);
    chartLayout->addWidget(m_priceChart);
    outer->addWidget(chartBox);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({"Data", "Sito / exchange", "Euro", "BTC on-chain", "Satoshi", "TX / ID transazione"});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    // Larghezze iniziali usate solo finché l'utente non le personalizza.
    // Dal secondo avvio vengono ripristinate esattamente quelle salvate.
    m_table->setColumnWidth(0, 110);
    m_table->setColumnWidth(1, 180);
    m_table->setColumnWidth(2, 120);
    m_table->setColumnWidth(3, 150);
    m_table->setColumnWidth(4, 140);
    m_table->setColumnWidth(5, 300);

    outer->addWidget(m_table, 1);

    auto *actions = new QHBoxLayout;
    auto *edit = new QPushButton("Modifica", this);
    auto *del = new QPushButton("Elimina", this);
    auto *imp = new QPushButton("Importa CSV", this);
    auto *expCsv = new QPushButton("Esporta CSV", this);
    auto *expPdf = new QPushButton("Esporta PDF", this);
    auto *backup = new QPushButton("Backup database", this);
    actions->addWidget(edit); actions->addWidget(del); actions->addSpacing(20);
    actions->addWidget(imp); actions->addWidget(expCsv); actions->addWidget(expPdf); actions->addStretch(); actions->addWidget(backup);
    outer->addLayout(actions);

    m_dbPath = new QLabel(this);
    m_dbPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QFont pf=m_dbPath->font(); pf.setPointSize(qMax(8, pf.pointSize()-1)); m_dbPath->setFont(pf);
    outer->addWidget(m_dbPath);

    setCentralWidget(central);

    auto *dbMenu = menuBar()->addMenu("Database");
    dbMenu->addAction("Mostra percorso", this, &MainWindow::showDatabasePath);
    dbMenu->addAction("Cambia cartella…", this, &MainWindow::changeDatabaseFolder);

    auto *infoMenu = menuBar()->addMenu("Info");
    infoMenu->addAction("BTC Purchase Tracker", this, &MainWindow::showAbout);

    connect(add, &QPushButton::clicked, this, &MainWindow::addPurchase);
    connect(edit, &QPushButton::clicked, this, &MainWindow::editPurchase);
    connect(del, &QPushButton::clicked, this, &MainWindow::deletePurchase);
    connect(imp, &QPushButton::clicked, this, &MainWindow::importCsv);
    connect(expCsv, &QPushButton::clicked, this, &MainWindow::exportCsv);
    connect(expPdf, &QPushButton::clicked, this, &MainWindow::exportPdf);
    connect(backup, &QPushButton::clicked, this, &MainWindow::backupDatabase);

    connect(m_yearFilter, &QComboBox::currentIndexChanged, this, [this](int) {
        refresh();
    });

    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int, int){ editPurchase(); });
}

void MainWindow::showAbout() {
    QDialog dialog(this);
    dialog.setWindowTitle("BTC Purchase Tracker");
    dialog.setModal(true);
    dialog.setMinimumWidth(500);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(26, 24, 26, 20);
    layout->setSpacing(14);

    auto *title = new QLabel("₿  BTC Purchase Tracker", &dialog);
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 5);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);

    auto *version = new QLabel("Versione 1.0.0", &dialog);
    version->setAlignment(Qt::AlignCenter);

    auto *description = new QLabel(
        "Un semplice tracker offline per registrare gli acquisti Bitcoin nel tempo.<br>"
        "Nessun account, nessun cloud, nessun collegamento al wallet.<br>"
        "I tuoi dati restano sul tuo computer.",
        &dialog
    );
    description->setTextFormat(Qt::RichText);
    description->setWordWrap(true);
    description->setAlignment(Qt::AlignCenter);

    auto *thanks = new QLabel(
        "Grazie per aver scaricato e utilizzato BTC Purchase Tracker.",
        &dialog
    );
    thanks->setWordWrap(true);
    thanks->setAlignment(Qt::AlignCenter);

    auto *contact = new QLabel(
        "Contatto: "
        "<a href=\"mailto:irql_not_less_or_equal@protonmail.com\">"
        "irql_not_less_or_equal@protonmail.com"
        "</a>",
        &dialog
    );
    contact->setTextFormat(Qt::RichText);
    contact->setTextInteractionFlags(Qt::TextBrowserInteraction);
    contact->setOpenExternalLinks(true);
    contact->setAlignment(Qt::AlignCenter);

    auto *disclaimer = new QLabel(
        "BTC Purchase Tracker non è un wallet e non fornisce consulenza finanziaria.",
        &dialog
    );
    disclaimer->setWordWrap(true);
    disclaimer->setAlignment(Qt::AlignCenter);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    layout->addWidget(title);
    layout->addWidget(version);
    layout->addSpacing(4);
    layout->addWidget(description);
    layout->addSpacing(4);
    layout->addWidget(thanks);
    layout->addWidget(contact);
    layout->addSpacing(4);
    layout->addWidget(disclaimer);
    layout->addSpacing(4);
    layout->addWidget(buttons);

    dialog.exec();
}

void MainWindow::refresh() {
    QString error;
    const auto allRows = m_db.purchases(&error);
    if (!error.isEmpty()) {
        QMessageBox::critical(this, "Errore database", error);
        return;
    }

    // Ricostruisce l'elenco degli anni presenti senza perdere il filtro selezionato.
    int selectedYear = 0;
    if (m_yearFilter)
        selectedYear = m_yearFilter->currentData().toInt();

    QSet<int> yearSet;
    for (const auto &p : allRows) {
        if (p.date.isValid())
            yearSet.insert(p.date.year());
    }

    QList<int> years = yearSet.values();
    std::sort(years.begin(), years.end(), std::greater<int>());

    {
        QSignalBlocker blocker(m_yearFilter);
        m_yearFilter->clear();
        m_yearFilter->addItem("Tutti gli anni", 0);

        for (const int year : years)
            m_yearFilter->addItem(QString::number(year), year);

        const int wantedIndex = m_yearFilter->findData(selectedYear);
        m_yearFilter->setCurrentIndex(wantedIndex >= 0 ? wantedIndex : 0);
    }

    selectedYear = m_yearFilter->currentData().toInt();

    QVector<Purchase> rows;
    rows.reserve(allRows.size());

    qint64 euro = 0;
    qint64 sats = 0;

    for (const auto &p : allRows) {
        if (selectedYear != 0 && p.date.year() != selectedYear)
            continue;

        rows.push_back(p);
        euro += p.euroCents;
        sats += p.sats;
    }

    m_table->setSortingEnabled(false);
    m_table->setRowCount(rows.size());

    for (int r = 0; r < rows.size(); ++r) {
        const auto &p = rows[r];

        auto set = [&](int c, const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            item->setData(Qt::UserRole, p.id);
            m_table->setItem(r, c, item);
        };

        auto setSortable = [&](int c, const QString &text, qint64 sortValue) {
            auto *item = new SortableNumberItem(text, sortValue);
            item->setTextAlignment(Qt::AlignCenter);
            item->setData(Qt::UserRole, p.id);
            m_table->setItem(r, c, item);
        };

        setSortable(0, p.date.toString("dd/MM/yyyy"), p.date.toJulianDay());
        set(1, p.site);
        setSortable(2, CsvUtils::formatEuro(p.euroCents), p.euroCents);
        setSortable(3, CsvUtils::satsToBtc(p.sats), p.sats);
        setSortable(4, CsvUtils::formatSats(p.sats), p.sats);
        set(5, p.txid);

    }

    m_table->setSortingEnabled(true);

    m_totalEuro->setText(CsvUtils::formatEuro(euro));
    m_totalBtc->setText(CsvUtils::satsToBtc(sats) + " BTC");
    m_totalSats->setText(CsvUtils::formatSats(sats) + " sats");

    // Prezzo medio di acquisto: euro investiti / BTC acquistati.
    // È solo un valore di visualizzazione: i dati salvati restano interi
    // (centesimi di euro e satoshi).
    double averageEurPerBtc = 0.0;

    if (sats > 0) {
        const long double average =
            static_cast<long double>(euro) * 1000000.0L /
            static_cast<long double>(sats);

        averageEurPerBtc = static_cast<double>(average);

        QLocale it(QLocale::Italian, QLocale::Italy);
        m_averagePrice->setText(
            it.toCurrencyString(averageEurPerBtc, "EUR")
        );

        m_averagePrice->setProperty(
            "clipboardValue",
            QString::number(averageEurPerBtc, 'f', 2)
                .replace('.', ',')
        );
    } else {
        m_averagePrice->setText("—");
        m_averagePrice->setProperty("clipboardValue", QString());
    }

    QVector<QPair<QDate, double>> chartPoints;
    chartPoints.reserve(rows.size());

    for (const auto &p : rows) {
        if (p.sats <= 0 || p.euroCents <= 0)
            continue;

        const long double purchasePrice =
            static_cast<long double>(p.euroCents) * 1000000.0L /
            static_cast<long double>(p.sats);

        chartPoints.append(
            qMakePair(p.date, static_cast<double>(purchasePrice))
        );
    }

    if (m_priceChart)
        m_priceChart->setData(chartPoints, averageEurPerBtc);

    // Valori puliti copiati negli appunti al clic sui totali.
    m_totalEuro->setProperty(
        "clipboardValue",
        QString::number(euro / 100.0, 'f', 2).replace('.', ',')
    );
    m_totalBtc->setProperty(
        "clipboardValue",
        CsvUtils::satsToBtc(sats)
    );
    m_totalSats->setProperty(
        "clipboardValue",
        QString::number(sats)
    );

    m_dbPath->setText("Database: " + m_db.filePath());
}

qint64 MainWindow::selectedId() const {
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) return -1;
    auto *item = m_table->item(rows.first().row(), 0);
    return item ? item->data(Qt::UserRole).toLongLong() : -1;
}

Purchase MainWindow::selectedPurchase() const {
    const qint64 id = selectedId();
    for (const auto &p : m_db.purchases()) if (p.id == id) return p;
    return {};
}

void MainWindow::addPurchase() {
    PurchaseDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
    Purchase p = dlg.purchase();
    if (!p.txid.isEmpty() && m_db.txidExists(p.txid)) {
        if (QMessageBox::question(this, "TX già presente", "Esiste già una riga con questo TX/ID. Vuoi salvarla comunque?") != QMessageBox::Yes) return;
    }
    QString error;
    if (!m_db.addPurchase(p, &error)) {
        QMessageBox::critical(this, "Errore", error);
        return;
    }

    QString csvError;
    if (!writeAutomaticCsvBackup(m_db, &csvError)) {
        QMessageBox::warning(
            this,
            "Backup CSV automatico",
            "L'operazione è stata salvata nel database, ma non è stato possibile "
            "aggiornare il CSV automatico:\n\n" + csvError
        );
    }
    refresh();
}

void MainWindow::editPurchase() {
    Purchase p = selectedPurchase();
    if (p.id < 0) { QMessageBox::information(this, "Selezione", "Seleziona prima un acquisto."); return; }
    PurchaseDialog dlg(this, &p);
    if (dlg.exec() != QDialog::Accepted) return;
    Purchase updated = dlg.purchase();
    if (!updated.txid.isEmpty() && m_db.txidExists(updated.txid, updated.id)) {
        if (QMessageBox::question(this, "TX già presente", "Esiste già un'altra riga con questo TX/ID. Vuoi salvare comunque?") != QMessageBox::Yes) return;
    }
    QString error;
    if (!m_db.updatePurchase(updated, &error)) {
        QMessageBox::critical(this, "Errore", error);
        return;
    }

    QString csvError;
    if (!writeAutomaticCsvBackup(m_db, &csvError)) {
        QMessageBox::warning(
            this,
            "Backup CSV automatico",
            "L'operazione è stata salvata nel database, ma non è stato possibile "
            "aggiornare il CSV automatico:\n\n" + csvError
        );
    }
    refresh();
}

void MainWindow::deletePurchase() {
    Purchase p = selectedPurchase();
    if (p.id < 0) { QMessageBox::information(this, "Selezione", "Seleziona prima un acquisto."); return; }
    if (QMessageBox::question(this, "Conferma eliminazione", QString("Eliminare l'acquisto del %1 su %2?").arg(p.date.toString("dd/MM/yyyy"), p.site)) != QMessageBox::Yes) return;
    QString error;
    if (!m_db.deletePurchase(p.id, &error)) {
        QMessageBox::critical(this, "Errore", error);
        return;
    }

    QString csvError;
    if (!writeAutomaticCsvBackup(m_db, &csvError)) {
        QMessageBox::warning(
            this,
            "Backup CSV automatico",
            "L'operazione è stata salvata nel database, ma non è stato possibile "
            "aggiornare il CSV automatico:\n\n" + csvError
        );
    }
    refresh();
}

void MainWindow::importCsv() {
    const QString path = QFileDialog::getOpenFileName(this, "Importa CSV", QString(), "File CSV (*.csv);;Tutti i file (*)");
    if (path.isEmpty()) return;
    const auto result = CsvUtils::importFile(path, m_db);
    if (result.validRows.isEmpty()) {
        QString msg = "Nessuna riga valida da importare.";
        if (!result.errors.isEmpty()) msg += "\n\n" + result.errors.mid(0,10).join("\n");
        QMessageBox::warning(this, "Importazione CSV", msg);
        return;
    }

    QString summary = QString("Righe valide: %1\nDuplicati TX/ID ignorati: %2\nRighe con errori: %3")
        .arg(result.validRows.size()).arg(result.duplicateRows).arg(result.errors.size());
    if (!result.errors.isEmpty()) summary += "\n\nPrimi errori:\n" + result.errors.mid(0,8).join("\n");
    summary += "\n\nImportare le righe valide?";
    if (QMessageBox::question(this, "Anteprima importazione CSV", summary) != QMessageBox::Yes) return;

    QString error;
    if (!m_db.addPurchasesTransaction(result.validRows, &error)) {
        QMessageBox::critical(this, "Errore importazione", error);
        return;
    }

    QString csvError;
    if (!writeAutomaticCsvBackup(m_db, &csvError)) {
        QMessageBox::warning(
            this,
            "Backup CSV automatico",
            "L'operazione è stata salvata nel database, ma non è stato possibile "
            "aggiornare il CSV automatico:\n\n" + csvError
        );
    }
    refresh();
    QMessageBox::information(this, "Importazione completata", QString("Importate %1 righe.").arg(result.validRows.size()));
}

void MainWindow::exportCsv() {
    const auto rows = m_db.purchases();
    if (rows.isEmpty()) { QMessageBox::information(this, "Esporta CSV", "Non ci sono acquisti da esportare."); return; }
    const QString path = QFileDialog::getSaveFileName(this, "Esporta CSV", "btc_acquisti.csv", "File CSV (*.csv)");
    if (path.isEmpty()) return;
    QSaveFile f(path.endsWith(".csv", Qt::CaseInsensitive) ? path : path + ".csv");
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) { QMessageBox::critical(this, "Errore", f.errorString()); return; }
    QTextStream out(&f); out.setEncoding(QStringConverter::Utf8);
    out << QChar(0xFEFF);
    const QChar d=';';
    out << "Data;Sito / exchange;Euro spesi;BTC on-chain;Satoshi;TX / ID transazione\n";
    for (const auto &p : rows) {
        out << p.date.toString("dd/MM/yyyy") << d
            << CsvUtils::csvEscape(p.site,d) << d
            << QString::number(p.euroCents/100.0,'f',2).replace('.',',') << d
            << CsvUtils::satsToBtc(p.sats).replace('.',',') << d
            << p.sats << d
            << CsvUtils::csvEscape(p.txid,d) << "\n";
    }
    const auto [euro,sats]=m_db.totals();
    out << "\nTOTALI;;" << QString::number(euro/100.0,'f',2).replace('.',',') << d
        << CsvUtils::satsToBtc(sats).replace('.',',') << d << sats << d << "\n";
    if (!f.commit()) { QMessageBox::critical(this, "Errore", f.errorString()); return; }
    QMessageBox::information(this, "Esportazione completata", "CSV salvato correttamente.");
}

void MainWindow::exportPdf() {
    const auto rows = m_db.purchases();
    if (rows.isEmpty()) { QMessageBox::information(this, "Esporta PDF", "Non ci sono acquisti da esportare."); return; }
    QString path = QFileDialog::getSaveFileName(this, "Esporta PDF", "btc_acquisti.pdf", "PDF (*.pdf)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".pdf", Qt::CaseInsensitive)) path += ".pdf";

    QPdfWriter writer(path);
    writer.setTitle("BTC Purchase Tracker - Report acquisti");
    writer.setCreator("BTC Purchase Tracker");
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(QPageLayout::Landscape);
    writer.setResolution(120);

    const auto [euro,sats] = m_db.totals();
    QString html = "<html><head><style>body{font-family:sans-serif;font-size:9pt;}h1{font-size:18pt;}"
                   "table{border-collapse:collapse;width:100%;}th,td{border:1px solid #aaa;padding:5px;}"
                   "th{background:#eee;}td.num{text-align:right;} .tot{font-size:11pt;margin:12px 0;}</style></head><body>";
    html += "<h1>BTC Purchase Tracker — Report acquisti</h1>";
    html += QString("<div class='tot'><b>Totale euro spesi:</b> %1 &nbsp;&nbsp; <b>Totale BTC:</b> %2 &nbsp;&nbsp; <b>Totale satoshi:</b> %3</div>")
        .arg(htmlEscape(CsvUtils::formatEuro(euro)), CsvUtils::satsToBtc(sats), htmlEscape(CsvUtils::formatSats(sats)));
    html += "<table><tr><th>Data</th><th>Sito / exchange</th><th>Euro</th><th>BTC on-chain</th><th>Satoshi</th><th>TX / ID transazione</th></tr>";
    for (const auto &p : rows) {
        html += QString("<tr><td>%1</td><td>%2</td><td class='num'>%3</td><td class='num'>%4</td><td class='num'>%5</td><td>%6</td></tr>")
            .arg(p.date.toString("dd/MM/yyyy"), htmlEscape(p.site), htmlEscape(CsvUtils::formatEuro(p.euroCents)),
                 CsvUtils::satsToBtc(p.sats), htmlEscape(CsvUtils::formatSats(p.sats)), htmlEscape(p.txid));
    }
    html += "</table>";
    html += QString("<p>Generato il %1</p></body></html>").arg(QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm"));

    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&writer);
    QMessageBox::information(this, "Esportazione completata", "PDF salvato correttamente.");
}

void MainWindow::backupDatabase() {
    const QString path = QFileDialog::getSaveFileName(this, "Backup database", "btc-purchase-tracker-backup.sqlite", "SQLite (*.sqlite *.db);;Tutti i file (*)");
    if (path.isEmpty()) return;
    if (QFile::exists(path) && !QFile::remove(path)) { QMessageBox::critical(this, "Errore", "Impossibile sovrascrivere il file di destinazione."); return; }
    if (!QFile::copy(m_db.filePath(), path)) { QMessageBox::critical(this, "Errore", "Impossibile copiare il database."); return; }
    QMessageBox::information(this, "Backup completato", "Backup creato correttamente.");
}

void MainWindow::showDatabasePath() {
    QMessageBox::information(this, "Percorso database", m_db.filePath());
}

void MainWindow::changeDatabaseFolder() {
    const QString oldPath = m_db.filePath();
    const QString folder = chooseDatabaseFolder("Scegli la nuova cartella del database", QFileInfo(oldPath).absolutePath());
    if (folder.isEmpty()) return;
    const QString newPath = QDir(folder).filePath(kDbName);
    if (QFileInfo(newPath).absoluteFilePath() == QFileInfo(oldPath).absoluteFilePath()) return;
    if (QFile::exists(newPath)) {
        QMessageBox::warning(this, "File già presente", "Nella cartella scelta esiste già un database con lo stesso nome. Operazione annullata.");
        return;
    }
    m_db.close();
    if (!QFile::copy(oldPath, newPath)) {
        QString error;
        m_db.open(oldPath, &error);
        QMessageBox::critical(this, "Errore", "Impossibile copiare il database nella nuova cartella.");
        return;
    }
    if (!openDatabaseAt(folder, true)) {
        QString error;
        m_db.open(oldPath, &error);
        return;
    }
    refresh();
    QMessageBox::information(this, "Database spostato", "Il database è stato copiato nella nuova cartella.\n\nIl vecchio file non è stato cancellato, così resta come copia di sicurezza.");
}
