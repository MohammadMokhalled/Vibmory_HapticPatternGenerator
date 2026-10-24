#include "ui/device_profile_actions.h"

#include "application/device_profiles.h"
#include "platform/device_profile_store.h"
#include "platform/project_store.h"
#include "ui/file_dialogs.h"

#include <QAction>
#include <QCheckBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>

#include <algorithm>

namespace vibmory
{
DeviceProfileActions::DeviceProfileActions(EditorSession& session,
                                           PlaybackBackend& backend,
                                           QMenu& menu,
                                           std::function<void()> changed,
                                           QWidget* parent)
    : QObject(parent), session_(session), backend_(backend),
      changed_(std::move(changed)), window_(parent),
      save_(menu.addAction(tr("Save device profile…"), this,
                           &DeviceProfileActions::saveProfile)),
      load_(menu.addAction(tr("Load device profile…"), this,
                           &DeviceProfileActions::loadProfile))
{
    save_->setObjectName("saveDeviceProfileAction");
    load_->setObjectName("loadDeviceProfileAction");
}

void DeviceProfileActions::setEnabled(bool enabled)
{
    save_->setEnabled(enabled);
    load_->setEnabled(enabled);
}

void DeviceProfileActions::saveProfile()
{
    bool accepted = false;
    const auto name = QInputDialog::getText(window_, tr("Save device profile"),
                                            tr("Profile name"),
                                            QLineEdit::Normal, {}, &accepted)
                          .trimmed();
    if (!accepted)
        return;
    const auto& document = session_.document();
    const DeviceProfile profile{name.toStdString(), document.shape(),
                                document.routing(),
                                backend_.selectedDeviceId()};
    if (const auto valid = profile.validate(); !valid)
    {
        QMessageBox::critical(window_, tr("Device profile"),
                              QString::fromStdString(valid.error().message));
        return;
    }
    const auto path = chooseSavePath(
        window_, tr("Save device profile"),
        tr("Vibmory device profiles (*.vibprofile)"), "vibprofile");
    if (path.isEmpty())
        return;
    const auto result = saveDeviceProfile(toPath(path), profile);
    if (!result)
        QMessageBox::critical(window_, tr("Save device profile"),
                              QString::fromStdString(result.error().message));
}

void DeviceProfileActions::loadProfile()
{
    const auto path = QFileDialog::getOpenFileName(
        window_, tr("Load device profile"), {},
        tr("Vibmory device profiles (*.vibprofile);;All files (*)"));
    if (path.isEmpty())
        return;
    const auto loaded = loadDeviceProfile(toPath(path));
    if (!loaded)
    {
        QMessageBox::critical(window_, tr("Load device profile"),
                              QString::fromStdString(loaded.error().message));
        return;
    }
    const auto& profile = loaded.value();
    if (profile.shape != session_.document().shape())
    {
        QMessageBox::warning(window_, tr("Device profile"),
                             tr("This profile uses %1 rows and %2 columns. Its "
                                "grid must match the current project.")
                                 .arg(profile.shape.rows)
                                 .arg(profile.shape.columns));
        return;
    }
    QMessageBox review(
        QMessageBox::Question, tr("Apply device profile"),
        tr("Apply “%1” to this project's channel routing and gains?")
            .arg(QString::fromStdString(profile.name)),
        QMessageBox::Apply | QMessageBox::Cancel, window_);
    review.setObjectName("deviceProfileReviewDialog");
    review.setTextFormat(Qt::PlainText);
    review.setDefaultButton(QMessageBox::Cancel);
    review.setInformativeText(tr(
        "Cell values and playback timing are preserved. Routing changes can be "
        "undone. Device selection is a machine preference and is not undone."));
    QStringList channels;
    for (std::size_t channel = 0; channel < profile.routing.cells.size();
         ++channel)
    {
        const auto cell = profile.routing.cells[channel];
        channels << tr("Channel %1 → R%2 C%3 · gain %4")
                        .arg(channel + 1)
                        .arg(cell / profile.shape.columns + 1)
                        .arg(cell % profile.shape.columns + 1)
                        .arg(profile.routing.gains[channel], 0, 'g', 8);
    }
    review.setDetailedText(channels.join('\n'));
    const auto devices = backend_.devices();
    const auto device =
        std::find_if(devices.begin(), devices.end(), [&](const auto& item) {
            return item.id == profile.outputDeviceId;
        });
    if (device != devices.end() && !profile.outputDeviceId.empty())
        review.setCheckBox(
            new QCheckBox(tr("Also select output device: %1")
                              .arg(QString::fromStdString(device->label))));
    else
        review.setInformativeText(
            review.informativeText() + "\n\n" +
            tr("The profile's preferred device is unavailable or unspecified. "
               "The current output device will be retained."));
    if (review.exec() != QMessageBox::Apply)
        return;
    const auto result =
        applyDeviceProfile(session_, backend_, profile,
                           review.checkBox() && review.checkBox()->isChecked());
    if (!result)
        QMessageBox::critical(window_, tr("Device profile"),
                              QString::fromStdString(result.error().message));
    changed_();
}
} // namespace vibmory
