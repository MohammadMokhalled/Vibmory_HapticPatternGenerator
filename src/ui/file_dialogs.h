#pragma once

#include <QString>

class QWidget;

namespace vibmory
{
// The dialog resolves the suffix before checking overwrite consent.
[[nodiscard]] QString chooseSavePath(QWidget* parent, const QString& title,
                                     const QString& filter,
                                     const QString& suffix,
                                     const QString& initial = {});
} // namespace vibmory
