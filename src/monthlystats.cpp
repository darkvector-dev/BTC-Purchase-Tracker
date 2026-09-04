#include "monthlystats.h"

#include <QMap>

namespace MonthlyStats {

MonthlySummary calculate(
    const QVector<Purchase> &purchases,
    int selectedYear,
    const QDate &today
) {
    MonthlySummary summary;

    QDate firstMonth;
    QDate lastMonth;

    if (selectedYear != 0) {
        firstMonth = QDate(selectedYear, 1, 1);
        const int lastMonthNumber = selectedYear == today.year()
            ? today.month()
            : 12;
        lastMonth = QDate(selectedYear, lastMonthNumber, 1);
    } else {
        for (const auto &purchase : purchases) {
            if (!purchase.date.isValid())
                continue;

            const QDate month(purchase.date.year(), purchase.date.month(), 1);
            if (!firstMonth.isValid() || month < firstMonth)
                firstMonth = month;
            if (!lastMonth.isValid() || month > lastMonth)
                lastMonth = month;
        }
    }

    if (!firstMonth.isValid() || !lastMonth.isValid() || firstMonth > lastMonth)
        return summary;

    QMap<QDate, qint64> amountsByMonth;
    for (const auto &purchase : purchases) {
        if (!purchase.date.isValid())
            continue;
        if (selectedYear != 0 && purchase.date.year() != selectedYear)
            continue;

        const QDate month(purchase.date.year(), purchase.date.month(), 1);
        amountsByMonth[month] += purchase.euroCents;
        summary.totalCents += purchase.euroCents;
    }

    for (QDate month = firstMonth; month <= lastMonth; month = month.addMonths(1)) {
        summary.months.push_back({month, amountsByMonth.value(month, 0)});
    }

    if (!summary.months.isEmpty()) {
        const qint64 count = summary.months.size();
        summary.averageCents = summary.totalCents / count;
        if ((summary.totalCents % count) * 2 >= count)
            ++summary.averageCents;
    }

    return summary;
}

}
