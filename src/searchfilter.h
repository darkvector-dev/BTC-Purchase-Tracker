#pragma once

#include "currency.h"
#include "database.h"

#include <QString>

namespace PurchaseSearch {

enum class Field {
    All,
    Date,
    Site,
    Amount,
    Bitcoin,
    Transaction
};

bool matches(
    const Purchase &purchase,
    const QString &query,
    Field field,
    AppCurrency::Currency currency
);

} // namespace PurchaseSearch
