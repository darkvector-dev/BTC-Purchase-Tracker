#include "diagnosticlog.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QSaveFile>
#include <QStandardPaths>
#include <QSysInfo>

namespace {
constexpr qint64 kMaxLogBytes = 200 * 1024;
constexpr auto kLogFileName = "btc-purchase-tracker.log";
constexpr auto kRotatedLogFileName = "btc-purchase-tracker.log.1";

QMutex &logMutex() {
    static QMutex mutex;
    return mutex;
}

QString logDirectory() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (path.isEmpty())
        path = QDir(QDir::tempPath()).filePath(QStringLiteral("BTCPurchaseTracker"));
    return path;
}

QString currentLogPath() {
    return QDir(logDirectory()).filePath(QString::fromLatin1(kLogFileName));
}

QString rotatedLogPath() {
    return QDir(logDirectory()).filePath(QString::fromLatin1(kRotatedLogFileName));
}

QString sanitize(QString text) {
    // Keep every event on one line and mask the user's home directory if an
    // operating-system error happens to include an absolute path.
    text.replace('\r', ' ');
    text.replace('\n', ' ');
    text = text.simplified();

    const QString home = QDir::cleanPath(QDir::homePath());
    if (!home.isEmpty()) {
        text.replace(home, QStringLiteral("<HOME>"), Qt::CaseInsensitive);
        text.replace(QDir::toNativeSeparators(home), QStringLiteral("<HOME>"), Qt::CaseInsensitive);
    }

    return text;
}

void rotateIfNeeded(qint64 incomingBytes) {
    const QString path = currentLogPath();
    const QFileInfo info(path);
    if (!info.exists() || info.size() + incomingBytes <= kMaxLogBytes)
        return;

    QFile::remove(rotatedLogPath());
    if (!QFile::rename(path, rotatedLogPath()))
        QFile::remove(path);
}

void writeLine(const char *level, const QString &message) {
    QMutexLocker locker(&logMutex());

    QDir dir(logDirectory());
    if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
        return;

    const QString line = QStringLiteral("%1 [%2] %3\n")
        .arg(
            QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
            QString::fromLatin1(level),
            sanitize(message)
        );
    const QByteArray bytes = line.toUtf8();

    rotateIfNeeded(bytes.size());

    QFile file(currentLogPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;

    file.write(bytes);
    file.flush();
}

QByteArray readFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.readAll();
}
}

namespace DiagnosticLog {

void initialize() {
    writeLine(
        "INFO",
        QStringLiteral("Application started | version=%1 | os=%2 | arch=%3 | qt=%4")
            .arg(
                QCoreApplication::applicationVersion(),
                QSysInfo::prettyProductName(),
                QSysInfo::currentCpuArchitecture(),
                QString::fromLatin1(qVersion())
            )
    );
}

void info(const QString &message) {
    writeLine("INFO", message);
}

void warning(const QString &message) {
    writeLine("WARN", message);
}

void error(const QString &message) {
    writeLine("ERROR", message);
}

bool exportSnapshot(const QString &destinationPath, QString *errorMessage) {
    QMutexLocker locker(&logMutex());

    QByteArray combined;
    const QByteArray previous = readFile(rotatedLogPath());
    const QByteArray current = readFile(currentLogPath());

    if (!previous.isEmpty()) {
        combined += "--- previous rotated log ---\n";
        combined += previous;
    }
    if (!current.isEmpty()) {
        if (!combined.isEmpty())
            combined += "--- current log ---\n";
        combined += current;
    }

    if (combined.size() > kMaxLogBytes) {
        combined = combined.right(kMaxLogBytes);
        const qsizetype firstNewline = combined.indexOf('\n');
        if (firstNewline >= 0 && firstNewline + 1 < combined.size())
            combined.remove(0, firstNewline + 1);
    }

    QSaveFile output(destinationPath);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = output.errorString();
        return false;
    }

    if (output.write(combined) != combined.size()) {
        if (errorMessage)
            *errorMessage = output.errorString();
        output.cancelWriting();
        return false;
    }

    if (!output.commit()) {
        if (errorMessage)
            *errorMessage = output.errorString();
        return false;
    }

    return true;
}

} // namespace DiagnosticLog
