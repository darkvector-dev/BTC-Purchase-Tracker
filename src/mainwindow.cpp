#include "mainwindow.h"
#include "csvutils.h"
#include "currency.h"
#include "language.h"
#include "purchasedialog.h"

#include <QAction>
#include <QActionGroup>
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
#include <QMenu>
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


namespace {
QString L(const char *italian, const char *english) {
    return AppLanguage::text(italian, english);
}
}

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

    void setCurrency(AppCurrency::Currency currency) {
        m_currency = currency;
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
                L("Nessun acquisto da visualizzare", "No purchases to display")
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
            const QString label = AppCurrency::formatMajor(price, m_currency, 0);
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
                L("Media ", "Average ") + AppCurrency::formatMajor(m_averagePrice, m_currency, 0) + "/BTC"
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
                + AppCurrency::formatMajor(m_points[nearestIndex].second, m_currency, 2)
                + "/BTC";

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
    AppCurrency::Currency m_currency{AppCurrency::Currency::Euro};
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

    const auto [amountCents, sats] = db.totals(&queryError);
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
    const bool usd = db.currency() == AppCurrency::Currency::UsDollar;
    const QString amountHeader = usd
        ? L("Dollari spesi (USD)", "USD spent")
        : L("Euro spesi", "Euro spent");

    out << L("Data", "Date") << delimiter
        << L("Sito / exchange", "Site / exchange") << delimiter
        << amountHeader << delimiter
        << "BTC on-chain" << delimiter
        << "Satoshi" << delimiter
        << L("TX / ID transazione", "TX / Transaction ID") << "\n";

    for (const auto &p : rows) {
        QString btc = CsvUtils::satsToBtc(p.sats);
        if (!usd) btc.replace('.', ',');

        out << p.date.toString("dd/MM/yyyy") << delimiter
            << CsvUtils::csvEscape(p.site, delimiter) << delimiter
            << AppCurrency::plainAmount(p.euroCents, db.currency()) << delimiter
            << btc << delimiter
            << p.sats << delimiter
            << CsvUtils::csvEscape(p.txid, delimiter) << "\n";
    }

    QString totalBtc = CsvUtils::satsToBtc(sats);
    if (!usd) totalBtc.replace('.', ',');

    out << "\n" << L("TOTALI", "TOTALS") << ";;"
        << AppCurrency::plainAmount(amountCents, db.currency()) << delimiter
        << totalBtc << delimiter
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
        setToolTip(L("Clicca per copiare", "Click to copy"));
    }

protected:
    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            const QString value = property("clipboardValue").toString();
            if (!value.isEmpty()) {
                QApplication::clipboard()->setText(value);

                if (auto *mainWindow = qobject_cast<QMainWindow *>(window())) {
                    mainWindow->statusBar()->showMessage(
                        L("✓ Copiato negli appunti", "✓ Copied to clipboard"),
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

bool MainWindow::chooseInitialCurrency(AppCurrency::Currency *currency) {
    if (!currency) return false;

    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(L("Valuta del database", "Database currency"));
    box.setText(L(
        "Scegli la valuta da usare per questo database.",
        "Choose the currency to use for this database."
    ));
    box.setInformativeText(L(
        "La scelta sarà permanente per questo database e non potrà essere modificata in seguito. "
        "Non verrà effettuata alcuna conversione tra euro e dollari.",
        "This choice is permanent for this database and cannot be changed later. "
        "No conversion between euros and US dollars will be performed."
    ));

    auto *euroButton = box.addButton(QString::fromUtf8("Euro (€)"), QMessageBox::AcceptRole);
    auto *usdButton = box.addButton(
        L("Dollaro USA ($)", "US Dollar ($)"),
        QMessageBox::AcceptRole
    );
    auto *cancelButton = box.addButton(QMessageBox::Cancel);

    // La scelta è permanente: Enter/Esc non devono selezionare per errore
    // una valuta. Il default sicuro è annullare.
    box.setDefaultButton(cancelButton);
    box.setEscapeButton(cancelButton);

    box.exec();

    if (box.clickedButton() == euroButton) {
        *currency = AppCurrency::Currency::Euro;
        return true;
    }
    if (box.clickedButton() == usdButton) {
        *currency = AppCurrency::Currency::UsDollar;
        return true;
    }
    return false;
}

bool MainWindow::initializeDatabase() {
    QSettings settings(kOrg, kApp);
    QString folder = settings.value(kDbKey).toString();
    if (folder.isEmpty() || !QDir(folder).exists()) {
        QMessageBox::information(
            this,
            L("Prima configurazione", "First setup"),
            L(
                "Scegli la cartella in cui vuoi conservare il database degli acquisti.\n\n"
                "Il file rimarrà in quella posizione anche aggiornando o spostando l'applicazione.",
                "Choose the folder where you want to store the purchase database.\n\n"
                "The file will remain in that location even if you update or move the application."
            )
        );
        folder = chooseDatabaseFolder(
            L("Scegli la cartella del database", "Choose the database folder")
        );
        if (folder.isEmpty()) return false;
    }
    return openDatabaseAt(folder, true);
}

bool MainWindow::openDatabaseAt(const QString &folder, bool remember) {
    QDir dir(folder);
    if (!dir.exists() && !dir.mkpath(".")) {
        QMessageBox::critical(
            this,
            L("Errore", "Error"),
            L(
                "Impossibile creare o accedere alla cartella scelta.",
                "Unable to create or access the selected folder."
            )
        );
        return false;
    }

    const QString path = dir.filePath(kDbName);
    const bool databaseExisted = QFile::exists(path);

    AppCurrency::Currency newDatabaseCurrency = AppCurrency::Currency::Euro;
    if (!databaseExisted && !chooseInitialCurrency(&newDatabaseCurrency))
        return false;

    QString error;
    if (!m_db.open(path, &error)) {
        QMessageBox::critical(this, L("Errore database", "Database error"), error);
        return false;
    }

    // I database creati dalle versioni precedenti erano esclusivamente EUR.
    // Alla prima apertura vengono marcati automaticamente senza chiedere nulla.
    if (!m_db.hasStoredCurrency()) {
        const AppCurrency::Currency currency = databaseExisted
            ? AppCurrency::Currency::Euro
            : newDatabaseCurrency;

        if (!m_db.setCurrency(currency, &error)) {
            QMessageBox::critical(this, L("Errore database", "Database error"), error);
            m_db.close();

            // Se la configurazione di un database appena creato non riesce,
            // eliminiamo il file vuoto/parziale. Al prossimo avvio verrà quindi
            // chiesta di nuovo la valuta invece di scambiarlo per un DB legacy.
            if (!databaseExisted)
                QFile::remove(path);

            return false;
        }
    }

    if (remember) {
        QSettings settings(kOrg, kApp);
        settings.setValue(kDbKey, QFileInfo(folder).absoluteFilePath());
    }

    // buildUi() viene eseguito prima dell'apertura del database: ora che la
    // valuta è nota aggiorniamo immediatamente tutte le etichette.
    applyLanguage();

    QString csvError;
    if (!writeAutomaticCsvBackup(m_db, &csvError)) {
        QMessageBox::warning(
            this,
            L("Backup CSV automatico", "Automatic CSV backup"),
            L(
                "Il database è stato aperto correttamente, ma non è stato possibile aggiornare il CSV automatico:\n\n",
                "The database was opened successfully, but the automatic CSV backup could not be updated:\n\n"
            ) + csvError
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
    m_addButton = new QPushButton(this);
    titleRow->addWidget(title);
    titleRow->addStretch();
    titleRow->addWidget(m_addButton);
    outer->addLayout(titleRow);

    auto *cards = new QHBoxLayout;
    auto makeCard = [&](QLabel **caption, QLabel **value) {
        auto *box = new QFrame(this);
        box->setFrameShape(QFrame::StyledPanel);
        auto *l = new QVBoxLayout(box);
        *caption = new QLabel(box);
        QFont cf=(*caption)->font(); cf.setBold(true); (*caption)->setFont(cf);
        *value = new ClickableValueLabel(box);
        (*value)->setText("—");
        QFont vf=(*value)->font(); vf.setPointSize(vf.pointSize()+4); vf.setBold(true); (*value)->setFont(vf);
        l->addWidget(*caption); l->addWidget(*value);
        cards->addWidget(box);
    };
    makeCard(&m_cardEuroCaption, &m_totalEuro);
    makeCard(&m_cardBtcCaption, &m_totalBtc);
    makeCard(&m_cardSatsCaption, &m_totalSats);
    makeCard(&m_cardAverageCaption, &m_averagePrice);
    outer->addLayout(cards);

    auto *filterRow = new QHBoxLayout;
    m_filterLabel = new QLabel(this);
    QFont ff = m_filterLabel->font();
    ff.setBold(true);
    m_filterLabel->setFont(ff);

    m_yearFilter = new QComboBox(this);
    m_yearFilter->setMinimumWidth(150);

    filterRow->addWidget(m_filterLabel);
    filterRow->addWidget(m_yearFilter);
    filterRow->addStretch();
    outer->addLayout(filterRow);

    auto *chartBox = new QFrame(this);
    chartBox->setFrameShape(QFrame::NoFrame);
    auto *chartLayout = new QVBoxLayout(chartBox);
    chartLayout->setContentsMargins(0, 0, 0, 0);
    chartLayout->setSpacing(5);

    m_chartTitle = new QLabel(chartBox);
    QFont chartTitleFont = m_chartTitle->font();
    chartTitleFont.setBold(true);
    m_chartTitle->setFont(chartTitleFont);

    m_priceChart = new PurchasePriceChart(chartBox);
    chartLayout->addWidget(m_chartTitle);
    chartLayout->addWidget(m_priceChart);
    outer->addWidget(chartBox);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
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
    m_editButton = new QPushButton(this);
    m_deleteButton = new QPushButton(this);
    m_importButton = new QPushButton(this);
    m_exportCsvButton = new QPushButton(this);
    m_exportPdfButton = new QPushButton(this);
    m_backupButton = new QPushButton(this);
    actions->addWidget(m_editButton); actions->addWidget(m_deleteButton); actions->addSpacing(20);
    actions->addWidget(m_importButton); actions->addWidget(m_exportCsvButton); actions->addWidget(m_exportPdfButton); actions->addStretch(); actions->addWidget(m_backupButton);
    outer->addLayout(actions);

    m_dbPath = new QLabel(this);
    m_dbPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QFont pf=m_dbPath->font(); pf.setPointSize(qMax(8, pf.pointSize()-1)); m_dbPath->setFont(pf);
    outer->addWidget(m_dbPath);

    setCentralWidget(central);

    m_databaseMenu = menuBar()->addMenu(QString());
    m_showPathAction = m_databaseMenu->addAction(QString(), this, &MainWindow::showDatabasePath);
    m_changeFolderAction = m_databaseMenu->addAction(QString(), this, &MainWindow::changeDatabaseFolder);

    m_settingsMenu = menuBar()->addMenu(QString());
    m_languageMenu = m_settingsMenu->addMenu(QString());
    m_italianAction = m_languageMenu->addAction("Italiano");
    m_englishAction = m_languageMenu->addAction("English");
    m_italianAction->setCheckable(true);
    m_englishAction->setCheckable(true);

    auto *languageGroup = new QActionGroup(this);
    languageGroup->setExclusive(true);
    languageGroup->addAction(m_italianAction);
    languageGroup->addAction(m_englishAction);

    m_infoMenu = menuBar()->addMenu(QString());
    m_aboutAction = m_infoMenu->addAction("BTC Purchase Tracker", this, &MainWindow::showAbout);

    connect(m_addButton, &QPushButton::clicked, this, &MainWindow::addPurchase);
    connect(m_editButton, &QPushButton::clicked, this, &MainWindow::editPurchase);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::deletePurchase);
    connect(m_importButton, &QPushButton::clicked, this, &MainWindow::importCsv);
    connect(m_exportCsvButton, &QPushButton::clicked, this, &MainWindow::exportCsv);
    connect(m_exportPdfButton, &QPushButton::clicked, this, &MainWindow::exportPdf);
    connect(m_backupButton, &QPushButton::clicked, this, &MainWindow::backupDatabase);

    connect(m_italianAction, &QAction::triggered, this, [this] {
        changeLanguage(false);
    });
    connect(m_englishAction, &QAction::triggered, this, [this] {
        changeLanguage(true);
    });

    connect(m_yearFilter, &QComboBox::currentIndexChanged, this, [this](int) {
        refresh();
    });

    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int, int){ editPurchase(); });

    applyLanguage();
}

void MainWindow::applyLanguage() {
    m_addButton->setText(L("+ Nuovo acquisto", "+ New purchase"));

    const bool usd = m_db.currency() == AppCurrency::Currency::UsDollar;
    m_cardEuroCaption->setText(usd
        ? L("DOLLARI SPESI", "USD SPENT")
        : L("EURO SPESI", "EURO SPENT"));
    m_cardBtcCaption->setText(L("BTC ACQUISTATI", "BTC PURCHASED"));
    m_cardSatsCaption->setText("SATOSHI");
    m_cardAverageCaption->setText(
        L("PREZZO MEDIO ", "AVERAGE PRICE ")
        + AppCurrency::pricePerBtcUnit(m_db.currency())
    );

    const QString copyTip = L("Clicca per copiare", "Click to copy");
    m_totalEuro->setToolTip(copyTip);
    m_totalBtc->setToolTip(copyTip);
    m_totalSats->setToolTip(copyTip);
    m_averagePrice->setToolTip(copyTip);

    m_filterLabel->setText(L("Anno:", "Year:"));
    m_yearFilter->setToolTip(L("Filtra la tabella e i totali per anno", "Filter the table and totals by year"));
    const int allYearsIndex = m_yearFilter->findData(0);
    if (allYearsIndex >= 0)
        m_yearFilter->setItemText(allYearsIndex, L("Tutti gli anni", "All years"));

    m_chartTitle->setText(L("ANDAMENTO PREZZO DI ACQUISTO", "PURCHASE PRICE TREND"));
    if (m_priceChart)
        m_priceChart->setCurrency(m_db.currency());

    m_table->setHorizontalHeaderLabels({
        L("Data", "Date"),
        L("Sito / exchange", "Site / exchange"),
        AppCurrency::code(m_db.currency()),
        "BTC on-chain",
        "Satoshi",
        L("TX / ID transazione", "TX / Transaction ID")
    });

    m_editButton->setText(L("Modifica", "Edit"));
    m_deleteButton->setText(L("Elimina", "Delete"));
    m_importButton->setText(L("Importa CSV", "Import CSV"));
    m_exportCsvButton->setText(L("Esporta CSV", "Export CSV"));
    m_exportPdfButton->setText(L("Esporta PDF", "Export PDF"));
    m_backupButton->setText(L("Backup database", "Database backup"));

    m_databaseMenu->setTitle("Database");
    m_showPathAction->setText(L("Mostra percorso", "Show path"));
    m_changeFolderAction->setText(L("Cambia cartella…", "Change folder…"));

    m_settingsMenu->setTitle(L("Impostazioni", "Settings"));
    m_languageMenu->setTitle(L("Lingua", "Language"));
    m_italianAction->setText("Italiano");
    m_englishAction->setText("English");
    m_italianAction->setChecked(!AppLanguage::isEnglish());
    m_englishAction->setChecked(AppLanguage::isEnglish());

    m_infoMenu->setTitle("Info");
    m_aboutAction->setText("BTC Purchase Tracker");

    if (!m_db.filePath().isEmpty())
        m_dbPath->setText("Database: " + m_db.filePath());
}

void MainWindow::changeLanguage(bool english) {
    const AppLanguage::Language language = english
        ? AppLanguage::Language::English
        : AppLanguage::Language::Italian;

    if (AppLanguage::current() == language) {
        applyLanguage();
        return;
    }

    AppLanguage::setCurrent(language);
    applyLanguage();
    refresh();
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

    auto *version = new QLabel(L("Versione 1.0.0", "Version 1.0.0"), &dialog);
    version->setAlignment(Qt::AlignCenter);

    auto *description = new QLabel(
        L(
            "Un semplice tracker offline per registrare gli acquisti Bitcoin nel tempo.<br>"
            "Nessun account, nessun cloud, nessun collegamento al wallet.<br>"
            "I tuoi dati restano sul tuo computer.",
            "A simple offline tracker for recording Bitcoin purchases over time.<br>"
            "No account, no cloud, no wallet connection.<br>"
            "Your data stays on your computer."
        ),
        &dialog
    );
    description->setTextFormat(Qt::RichText);
    description->setWordWrap(true);
    description->setAlignment(Qt::AlignCenter);

    auto *thanks = new QLabel(
        L(
            "Grazie per aver scaricato e utilizzato BTC Purchase Tracker.",
            "Thank you for downloading and using BTC Purchase Tracker."
        ),
        &dialog
    );
    thanks->setWordWrap(true);
    thanks->setAlignment(Qt::AlignCenter);

    auto *contact = new QLabel(
        L("Contatto: ", "Contact: ")
        + QStringLiteral("<a href=\"mailto:irql_not_less_or_equal@protonmail.com\">"
                         "irql_not_less_or_equal@protonmail.com"
                         "</a>"),
        &dialog
    );
    contact->setTextFormat(Qt::RichText);
    contact->setTextInteractionFlags(Qt::TextBrowserInteraction);
    contact->setOpenExternalLinks(true);
    contact->setAlignment(Qt::AlignCenter);

    auto *disclaimer = new QLabel(
        L(
            "BTC Purchase Tracker non è un wallet e non fornisce consulenza finanziaria.",
            "BTC Purchase Tracker is not a wallet and does not provide financial advice."
        ),
        &dialog
    );
    disclaimer->setWordWrap(true);
    disclaimer->setAlignment(Qt::AlignCenter);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    buttons->button(QDialogButtonBox::Close)->setText(L("Chiudi", "Close"));
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
        QMessageBox::critical(this, L("Errore database", "Database error"), error);
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
        m_yearFilter->addItem(L("Tutti gli anni", "All years"), 0);

        for (const int year : years)
            m_yearFilter->addItem(QString::number(year), year);

        const int wantedIndex = m_yearFilter->findData(selectedYear);
        m_yearFilter->setCurrentIndex(wantedIndex >= 0 ? wantedIndex : 0);
    }

    selectedYear = m_yearFilter->currentData().toInt();

    QVector<Purchase> rows;
    rows.reserve(allRows.size());

    qint64 amountCents = 0;
    qint64 sats = 0;

    for (const auto &p : allRows) {
        if (selectedYear != 0 && p.date.year() != selectedYear)
            continue;

        rows.push_back(p);
        amountCents += p.euroCents;
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
        setSortable(2, CsvUtils::formatMoney(p.euroCents, m_db.currency()), p.euroCents);
        setSortable(3, CsvUtils::satsToBtc(p.sats), p.sats);
        setSortable(4, CsvUtils::formatSats(p.sats), p.sats);
        set(5, p.txid);

    }

    m_table->setSortingEnabled(true);

    m_totalEuro->setText(CsvUtils::formatMoney(amountCents, m_db.currency()));
    m_totalBtc->setText(CsvUtils::satsToBtc(sats) + " BTC");
    m_totalSats->setText(CsvUtils::formatSats(sats) + " sats");

    // Prezzo medio di acquisto: valuta investita / BTC acquistati.
    // È solo un valore di visualizzazione: i dati salvati restano interi
    // (centesimi della valuta scelta e satoshi).
    double averageFiatPerBtc = 0.0;

    if (sats > 0) {
        const long double average =
            static_cast<long double>(amountCents) * 1000000.0L /
            static_cast<long double>(sats);

        averageFiatPerBtc = static_cast<double>(average);

        m_averagePrice->setText(
            AppCurrency::formatMajor(averageFiatPerBtc, m_db.currency(), 2)
        );

        QString averageClipboard = QString::number(averageFiatPerBtc, 'f', 2);
        if (m_db.currency() == AppCurrency::Currency::Euro)
            averageClipboard.replace('.', ',');
        m_averagePrice->setProperty("clipboardValue", averageClipboard);
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
        m_priceChart->setData(chartPoints, averageFiatPerBtc);

    // Valori puliti copiati negli appunti al clic sui totali.
    m_totalEuro->setProperty(
        "clipboardValue",
        AppCurrency::plainAmount(amountCents, m_db.currency())
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
    PurchaseDialog dlg(this, m_db.currency());
    if (dlg.exec() != QDialog::Accepted) return;
    Purchase p = dlg.purchase();
    if (!p.txid.isEmpty() && m_db.txidExists(p.txid)) {
        if (QMessageBox::question(
                this,
                L("TX già presente", "TX already exists"),
                L(
                    "Esiste già una riga con questo TX/ID. Vuoi salvarla comunque?",
                    "A row with this TX/ID already exists. Do you want to save it anyway?"
                )
            ) != QMessageBox::Yes) return;
    }
    QString error;
    if (!m_db.addPurchase(p, &error)) {
        QMessageBox::critical(this, L("Errore", "Error"), error);
        return;
    }

    QString csvError;
    if (!writeAutomaticCsvBackup(m_db, &csvError)) {
        QMessageBox::warning(
            this,
            L("Backup CSV automatico", "Automatic CSV backup"),
            L(
                "L'operazione è stata salvata nel database, ma non è stato possibile aggiornare il CSV automatico:\n\n",
                "The operation was saved to the database, but the automatic CSV backup could not be updated:\n\n"
            ) + csvError
        );
    }
    refresh();
}

void MainWindow::editPurchase() {
    Purchase p = selectedPurchase();
    if (p.id < 0) {
        QMessageBox::information(
            this,
            L("Selezione", "Selection"),
            L("Seleziona prima un acquisto.", "Select a purchase first.")
        );
        return;
    }

    PurchaseDialog dlg(this, m_db.currency(), &p);
    if (dlg.exec() != QDialog::Accepted) return;
    Purchase updated = dlg.purchase();

    if (!updated.txid.isEmpty() && m_db.txidExists(updated.txid, updated.id)) {
        if (QMessageBox::question(
                this,
                L("TX già presente", "TX already exists"),
                L(
                    "Esiste già un'altra riga con questo TX/ID. Vuoi salvare comunque?",
                    "Another row with this TX/ID already exists. Do you want to save it anyway?"
                )
            ) != QMessageBox::Yes) return;
    }

    QString error;
    if (!m_db.updatePurchase(updated, &error)) {
        QMessageBox::critical(this, L("Errore", "Error"), error);
        return;
    }

    QString csvError;
    if (!writeAutomaticCsvBackup(m_db, &csvError)) {
        QMessageBox::warning(
            this,
            L("Backup CSV automatico", "Automatic CSV backup"),
            L(
                "L'operazione è stata salvata nel database, ma non è stato possibile aggiornare il CSV automatico:\n\n",
                "The operation was saved to the database, but the automatic CSV backup could not be updated:\n\n"
            ) + csvError
        );
    }
    refresh();
}

void MainWindow::deletePurchase() {
    Purchase p = selectedPurchase();
    if (p.id < 0) {
        QMessageBox::information(
            this,
            L("Selezione", "Selection"),
            L("Seleziona prima un acquisto.", "Select a purchase first.")
        );
        return;
    }

    const QString question = AppLanguage::isEnglish()
        ? QString("Delete the purchase from %1 on %2?").arg(p.date.toString("dd/MM/yyyy"), p.site)
        : QString("Eliminare l'acquisto del %1 su %2?").arg(p.date.toString("dd/MM/yyyy"), p.site);

    if (QMessageBox::question(
            this,
            L("Conferma eliminazione", "Confirm deletion"),
            question
        ) != QMessageBox::Yes) return;

    QString error;
    if (!m_db.deletePurchase(p.id, &error)) {
        QMessageBox::critical(this, L("Errore", "Error"), error);
        return;
    }

    QString csvError;
    if (!writeAutomaticCsvBackup(m_db, &csvError)) {
        QMessageBox::warning(
            this,
            L("Backup CSV automatico", "Automatic CSV backup"),
            L(
                "L'operazione è stata salvata nel database, ma non è stato possibile aggiornare il CSV automatico:\n\n",
                "The operation was saved to the database, but the automatic CSV backup could not be updated:\n\n"
            ) + csvError
        );
    }
    refresh();
}

void MainWindow::importCsv() {
    const QString filter = L("File CSV (*.csv);;Tutti i file (*)", "CSV files (*.csv);;All files (*)");
    const QString path = QFileDialog::getOpenFileName(
        this,
        L("Importa CSV", "Import CSV"),
        QString(),
        filter
    );
    if (path.isEmpty()) return;

    const auto result = CsvUtils::importFile(path, m_db);
    if (result.validRows.isEmpty()) {
        QString msg = L("Nessuna riga valida da importare.", "No valid rows to import.");
        if (!result.errors.isEmpty()) msg += "\n\n" + result.errors.mid(0,10).join("\n");
        QMessageBox::warning(this, L("Importazione CSV", "CSV import"), msg);
        return;
    }

    QString summary = AppLanguage::isEnglish()
        ? QString("Valid rows: %1\nDuplicate TX/IDs ignored: %2\nRows with errors: %3")
              .arg(result.validRows.size()).arg(result.duplicateRows).arg(result.errors.size())
        : QString("Righe valide: %1\nDuplicati TX/ID ignorati: %2\nRighe con errori: %3")
              .arg(result.validRows.size()).arg(result.duplicateRows).arg(result.errors.size());

    if (!result.errors.isEmpty())
        summary += L("\n\nPrimi errori:\n", "\n\nFirst errors:\n") + result.errors.mid(0,8).join("\n");

    summary += L("\n\nImportare le righe valide?", "\n\nImport the valid rows?");

    if (QMessageBox::question(
            this,
            L("Anteprima importazione CSV", "CSV import preview"),
            summary
        ) != QMessageBox::Yes) return;

    QString error;
    if (!m_db.addPurchasesTransaction(result.validRows, &error)) {
        QMessageBox::critical(this, L("Errore importazione", "Import error"), error);
        return;
    }

    QString csvError;
    if (!writeAutomaticCsvBackup(m_db, &csvError)) {
        QMessageBox::warning(
            this,
            L("Backup CSV automatico", "Automatic CSV backup"),
            L(
                "L'operazione è stata salvata nel database, ma non è stato possibile aggiornare il CSV automatico:\n\n",
                "The operation was saved to the database, but the automatic CSV backup could not be updated:\n\n"
            ) + csvError
        );
    }

    refresh();
    QMessageBox::information(
        this,
        L("Importazione completata", "Import complete"),
        AppLanguage::isEnglish()
            ? QString("Imported %1 rows.").arg(result.validRows.size())
            : QString("Importate %1 righe.").arg(result.validRows.size())
    );
}

void MainWindow::exportCsv() {
    const auto rows = m_db.purchases();
    if (rows.isEmpty()) {
        QMessageBox::information(
            this,
            L("Esporta CSV", "Export CSV"),
            L("Non ci sono acquisti da esportare.", "There are no purchases to export.")
        );
        return;
    }

    const QString defaultName = AppLanguage::isEnglish()
        ? QStringLiteral("btc_purchases.csv")
        : QStringLiteral("btc_acquisti.csv");

    const QString path = QFileDialog::getSaveFileName(
        this,
        L("Esporta CSV", "Export CSV"),
        defaultName,
        L("File CSV (*.csv)", "CSV files (*.csv)")
    );
    if (path.isEmpty()) return;

    QSaveFile f(path.endsWith(".csv", Qt::CaseInsensitive) ? path : path + ".csv");
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, L("Errore", "Error"), f.errorString());
        return;
    }

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << QChar(0xFEFF);
    const QChar d=';';

    const bool usd = m_db.currency() == AppCurrency::Currency::UsDollar;
    const QString amountHeader = usd
        ? L("Dollari spesi (USD)", "USD spent")
        : L("Euro spesi", "Euro spent");

    out << L("Data", "Date") << d
        << L("Sito / exchange", "Site / exchange") << d
        << amountHeader << d
        << "BTC on-chain" << d
        << "Satoshi" << d
        << L("TX / ID transazione", "TX / Transaction ID") << "\n";

    for (const auto &p : rows) {
        QString btc = CsvUtils::satsToBtc(p.sats);
        if (!usd) btc.replace('.', ',');

        out << p.date.toString("dd/MM/yyyy") << d
            << CsvUtils::csvEscape(p.site,d) << d
            << AppCurrency::plainAmount(p.euroCents, m_db.currency()) << d
            << btc << d
            << p.sats << d
            << CsvUtils::csvEscape(p.txid,d) << "\n";
    }

    const auto [amountCents,sats]=m_db.totals();
    QString totalBtc = CsvUtils::satsToBtc(sats);
    if (!usd) totalBtc.replace('.', ',');

    out << "\n" << L("TOTALI", "TOTALS") << ";;"
        << AppCurrency::plainAmount(amountCents, m_db.currency()) << d
        << totalBtc << d
        << sats << d << "\n";

    if (!f.commit()) {
        QMessageBox::critical(this, L("Errore", "Error"), f.errorString());
        return;
    }

    QMessageBox::information(
        this,
        L("Esportazione completata", "Export complete"),
        L("CSV salvato correttamente.", "CSV saved successfully.")
    );
}

void MainWindow::exportPdf() {
    const auto rows = m_db.purchases();
    if (rows.isEmpty()) {
        QMessageBox::information(
            this,
            L("Esporta PDF", "Export PDF"),
            L("Non ci sono acquisti da esportare.", "There are no purchases to export.")
        );
        return;
    }

    const QString defaultName = AppLanguage::isEnglish()
        ? QStringLiteral("btc_purchases.pdf")
        : QStringLiteral("btc_acquisti.pdf");

    QString path = QFileDialog::getSaveFileName(
        this,
        L("Esporta PDF", "Export PDF"),
        defaultName,
        "PDF (*.pdf)"
    );
    if (path.isEmpty()) return;
    if (!path.endsWith(".pdf", Qt::CaseInsensitive)) path += ".pdf";

    QPdfWriter writer(path);
    writer.setTitle(L(
        "BTC Purchase Tracker - Report acquisti",
        "BTC Purchase Tracker - Purchase report"
    ));
    writer.setCreator("BTC Purchase Tracker");
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(QPageLayout::Landscape);
    writer.setResolution(120);

    const auto [amountCents,sats] = m_db.totals();
    QString html = "<html><head><style>body{font-family:sans-serif;font-size:9pt;}h1{font-size:18pt;}"
                   "table{border-collapse:collapse;width:100%;}th,td{border:1px solid #aaa;padding:5px;}"
                   "th{background:#eee;}td.num{text-align:right;} .tot{font-size:11pt;margin:12px 0;}</style></head><body>";

    html += "<h1>" + L(
        "BTC Purchase Tracker — Report acquisti",
        "BTC Purchase Tracker — Purchase report"
    ) + "</h1>";

    html += QString("<div class='tot'><b>%1:</b> %2 &nbsp;&nbsp; <b>%3:</b> %4 &nbsp;&nbsp; <b>%5:</b> %6</div>")
        .arg(
            m_db.currency() == AppCurrency::Currency::UsDollar
                ? L("Totale dollari spesi", "Total USD spent")
                : L("Totale euro spesi", "Total euro spent"),
            htmlEscape(CsvUtils::formatMoney(amountCents, m_db.currency())),
            L("Totale BTC", "Total BTC"),
            CsvUtils::satsToBtc(sats),
            L("Totale satoshi", "Total satoshi"),
            htmlEscape(CsvUtils::formatSats(sats))
        );

    html += QString("<table><tr><th>%1</th><th>%2</th><th>%3</th><th>BTC on-chain</th><th>Satoshi</th><th>%4</th></tr>")
        .arg(
            L("Data", "Date"),
            L("Sito / exchange", "Site / exchange"),
            AppCurrency::code(m_db.currency()),
            L("TX / ID transazione", "TX / Transaction ID")
        );

    for (const auto &p : rows) {
        html += QString("<tr><td>%1</td><td>%2</td><td class='num'>%3</td><td class='num'>%4</td><td class='num'>%5</td><td>%6</td></tr>")
            .arg(
                p.date.toString("dd/MM/yyyy"),
                htmlEscape(p.site),
                htmlEscape(CsvUtils::formatMoney(p.euroCents, m_db.currency())),
                CsvUtils::satsToBtc(p.sats),
                htmlEscape(CsvUtils::formatSats(p.sats)),
                htmlEscape(p.txid)
            );
    }

    html += "</table>";
    html += QString("<p>%1 %2</p></body></html>")
        .arg(
            L("Generato il", "Generated on"),
            QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm")
        );

    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&writer);

    QMessageBox::information(
        this,
        L("Esportazione completata", "Export complete"),
        L("PDF salvato correttamente.", "PDF saved successfully.")
    );
}

void MainWindow::backupDatabase() {
    const QString path = QFileDialog::getSaveFileName(
        this,
        L("Backup database", "Database backup"),
        "btc-purchase-tracker-backup.sqlite",
        L("SQLite (*.sqlite *.db);;Tutti i file (*)", "SQLite (*.sqlite *.db);;All files (*)")
    );
    if (path.isEmpty()) return;

    if (QFile::exists(path) && !QFile::remove(path)) {
        QMessageBox::critical(
            this,
            L("Errore", "Error"),
            L(
                "Impossibile sovrascrivere il file di destinazione.",
                "Unable to overwrite the destination file."
            )
        );
        return;
    }

    if (!QFile::copy(m_db.filePath(), path)) {
        QMessageBox::critical(
            this,
            L("Errore", "Error"),
            L("Impossibile copiare il database.", "Unable to copy the database.")
        );
        return;
    }

    QMessageBox::information(
        this,
        L("Backup completato", "Backup complete"),
        L("Backup creato correttamente.", "Backup created successfully.")
    );
}

void MainWindow::showDatabasePath() {
    QMessageBox::information(
        this,
        L("Percorso database", "Database path"),
        m_db.filePath()
    );
}

void MainWindow::changeDatabaseFolder() {
    const QString oldPath = m_db.filePath();
    const QString folder = chooseDatabaseFolder(
        L("Scegli la nuova cartella del database", "Choose the new database folder"),
        QFileInfo(oldPath).absolutePath()
    );
    if (folder.isEmpty()) return;

    const QString newPath = QDir(folder).filePath(kDbName);
    if (QFileInfo(newPath).absoluteFilePath() == QFileInfo(oldPath).absoluteFilePath()) return;

    if (QFile::exists(newPath)) {
        QMessageBox::warning(
            this,
            L("File già presente", "File already exists"),
            L(
                "Nella cartella scelta esiste già un database con lo stesso nome. Operazione annullata.",
                "A database with the same name already exists in the selected folder. Operation cancelled."
            )
        );
        return;
    }

    m_db.close();
    if (!QFile::copy(oldPath, newPath)) {
        QString error;
        m_db.open(oldPath, &error);
        QMessageBox::critical(
            this,
            L("Errore", "Error"),
            L(
                "Impossibile copiare il database nella nuova cartella.",
                "Unable to copy the database to the new folder."
            )
        );
        return;
    }

    if (!openDatabaseAt(folder, true)) {
        QString error;
        m_db.open(oldPath, &error);
        return;
    }

    refresh();
    QMessageBox::information(
        this,
        L("Database spostato", "Database moved"),
        L(
            "Il database è stato copiato nella nuova cartella.\n\nIl vecchio file non è stato cancellato, così resta come copia di sicurezza.",
            "The database was copied to the new folder.\n\nThe old file was not deleted, so it remains as a safety copy."
        )
    );
}
