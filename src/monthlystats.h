#pragma once

#include "database.h"

#include <QDate>
#include <QVector>

struct MonthlySpend {
    QDate month;
    qint64 amountCents = 0;
};

struct MonthlySummary {
    QVector<MonthlySpend> months;
    qint64 totalCents = 0;
    qint64 averageCents = 0;
};

namespace MonthlyStats {
MonthlySummary calculate(
    const QVector<Purchase> &purchases,
    int selectedYear,
    const QDate &today = QDate::currentDate()
);
}
