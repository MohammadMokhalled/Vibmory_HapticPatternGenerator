#include "ui/file_dialogs.h"

#include <QFileDialog>
#include <QStringList>

namespace vibmory
{
QString chooseSavePath(QWidget* parent, const QString& title,
                       const QString& filter, const QString& suffix,
                       const QString& initial)
{
    QFileDialog dialog(parent, title, initial, filter);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setDefaultSuffix(suffix);
    dialog.setOption(QFileDialog::DontConfirmOverwrite, false);
    if (dialog.exec() != QDialog::Accepted)
        return {};
    const auto files = dialog.selectedFiles();
    return files.isEmpty() ? QString{} : files.first();
}
} // namespace vibmory
