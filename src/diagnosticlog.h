#pragma once

#include <QString>

namespace DiagnosticLog {

// Starts the local diagnostic log. The log never contains purchase amounts,
// BTC/sats values, notes, transaction IDs or the BOLT12 offer.
void initialize();

void info(const QString &message);
void warning(const QString &message);
void error(const QString &message);

// Exports the most recent diagnostic history to a user-selected file.
// The exported file is capped at 200 KiB even when rotation has occurred.
bool exportSnapshot(const QString &destinationPath, QString *errorMessage = nullptr);

} // namespace DiagnosticLog
