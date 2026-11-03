#include "ui/recovery_controller.h"

#include "platform/project_store.h"

#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QtConcurrentRun>

#include <algorithm>
#include <exception>

namespace vibmory
{
namespace
{
Q_LOGGING_CATEGORY(recoveryLog, "vibmory.recovery")
}

RecoveryController::RecoveryController(EditorSession& session,
                                       QString directory,
                                       RecoveryWriteOperation write,
                                       QObject* parent)
    : QObject(parent), session_(session), directory_(std::move(directory)),
      write_(std::move(write)), generation_(session.documentGeneration())
{
    timer_.setInterval(30000);
    connect(&timer_, &QTimer::timeout, this, &RecoveryController::captureNow);
    connect(&watcher_, &QFutureWatcher<Result<void>>::finished, this, [this] {
        // A completion queued before clear()/restart must not consume an active
        // replacement job or block waiting for it on the GUI thread.
        if (!state_ || !watcher_.isFinished())
            return;
        const auto result = watcher_.result();
        const auto revision = state_->revision;
        state_.reset();
        if (result)
        {
            writtenRevision_ = revision;
            emit snapshotSaved(revision);
        }
        else if (result.error().code != ErrorCode::Cancelled)
            report(result.error());
    });
}

RecoveryController::~RecoveryController()
{
    clear();
}

void RecoveryController::report(const Error& error)
{
    qCWarning(recoveryLog).noquote() << QString::fromStdString(error.message);
    emit warningOccurred(QString::fromStdString(error.message));
}

void RecoveryController::stopAndWait()
{
    timer_.stop();
    if (state_)
        state_->cancel = true;
    watcher_.waitForFinished();
    state_.reset();
}

void RecoveryController::clear()
{
    stopAndWait();
    if (file_)
    {
        if (auto result = file_->discard(); !result)
            report(result.error());
        file_.reset();
    }
    writtenRevision_.reset();
}

void RecoveryController::observe()
{
    if (generation_ != session_.documentGeneration())
    {
        clear();
        generation_ = session_.documentGeneration();
    }
    if (!session_.dirty())
        clear();
    else if (!timer_.isActive())
        timer_.start();
}

void RecoveryController::captureNow()
{
    observe();
    if (!session_.dirty() || busy() || writtenRevision_ == session_.revision())
        return;
    if (!write_)
    {
        report(invalid("A recovery write operation is required."));
        return;
    }
    if (!file_)
    {
        auto created = RecoveryFile::create(directory_);
        if (!created)
        {
            report(created.error());
            return;
        }
        file_ = std::move(created.value());
    }
    state_ = std::make_shared<State>();
    state_->revision = session_.revision();
    const auto state = state_;
    watcher_.setFuture(QtConcurrent::run(
        [state, write = write_, path = toPath(file_->path()),
         record = RecoveryRecord{
             session_.document(), session_.path(),
             QDateTime::currentDateTimeUtc()}]() -> Result<void> {
            try
            {
                return write(path, record,
                             [state] { return !state->cancel.load(); });
            }
            catch (const std::exception& error)
            {
                return Error{ErrorCode::Io,
                             std::string("Recovery write failed: ") +
                                 error.what()};
            }
            catch (...)
            {
                return Error{ErrorCode::Io,
                             "Recovery write failed unexpectedly."};
            }
        }));
}

bool RecoveryController::offerRestore(QWidget* parent)
{
    if (session_.dirty() || busy())
        return false;
    auto found = RecoveryFile::available(directory_);
    if (!found)
    {
        report(found.error());
        return false;
    }
    auto files = std::move(found.value());
    if (files.empty())
        return false;
    QDialog dialog(parent);
    dialog.setObjectName("recoveryDialog");
    dialog.setWindowTitle(tr("Recover unsaved work"));
    dialog.resize(650, 350);
    auto* layout = new QVBoxLayout(&dialog);
    auto* explanation = new QLabel(
        tr("These snapshots belong to earlier sessions that did not close "
           "normally. Restore opens unsaved work; your project file is "
           "preserved. Keep leaves snapshots for a later launch."));
    explanation->setWordWrap(true);
    layout->addWidget(explanation);
    auto* list = new QListWidget;
    list->setObjectName("recoveryList");
    list->setAccessibleName(tr("Recoverable projects"));
    layout->addWidget(list);
    std::vector<Result<RecoveryRecord>> records;
    for (const auto& file : files)
    {
        records.push_back(readRecoveryFile(toPath(file->path())));
        const auto& record = records.back();
        list->addItem(
            record ? tr("%1 — %2")
                         .arg(record.value().originalPath.empty()
                                  ? tr("Untitled")
                                  : toQString(record.value().originalPath))
                         .arg(record.value().capturedAt.toLocalTime().toString(
                             Qt::ISODate))
                   : tr("Unreadable snapshot — %1")
                         .arg(QFileInfo(file->path()).fileName()));
    }
    auto* buttons = new QDialogButtonBox;
    auto* restore =
        buttons->addButton(tr("Restore"), QDialogButtonBox::AcceptRole);
    restore->setObjectName("restoreRecoveryButton");
    auto* discard = buttons->addButton(tr("Discard selected"),
                                       QDialogButtonBox::DestructiveRole);
    discard->setObjectName("discardRecoveryButton");
    auto* keep =
        buttons->addButton(tr("Keep for later"), QDialogButtonBox::RejectRole);
    keep->setObjectName("keepRecoveryButton");
    keep->setDefault(true);
    layout->addWidget(buttons);
    connect(list, &QListWidget::currentRowChanged, &dialog, [&](int row) {
        restore->setEnabled(
            row >= 0 &&
            static_cast<bool>(records[static_cast<std::size_t>(row)]));
        discard->setEnabled(row >= 0);
    });
    list->setCurrentRow(0);
    bool restored = false;
    connect(keep, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(discard, &QPushButton::clicked, &dialog, [&] {
        const auto row = list->currentRow();
        if (row < 0)
            return;
        const auto index = static_cast<std::size_t>(row);
        if (const auto result = files[index]->discard(); !result)
        {
            QMessageBox::warning(
                &dialog, tr("Recovery"),
                QString::fromStdString(result.error().message));
            return;
        }
        // Block row changes while removing parallel entries, then select again.
        const QSignalBlocker blocker(list);
        delete list->takeItem(row);
        files.erase(files.begin() + row);
        records.erase(records.begin() + row);
        if (files.empty())
            dialog.reject();
        else
        {
            const auto selected = std::min(row, list->count() - 1);
            list->setCurrentRow(selected);
            restore->setEnabled(
                static_cast<bool>(records[static_cast<std::size_t>(selected)]));
        }
    });
    connect(restore, &QPushButton::clicked, &dialog, [&] {
        const auto row = list->currentRow();
        if (row < 0 || !records[static_cast<std::size_t>(row)])
            return;
        auto& record = records[static_cast<std::size_t>(row)].value();
        clear();
        session_.restore(std::move(record.document),
                         std::move(record.originalPath));
        generation_ = session_.documentGeneration();
        file_ = std::move(files[static_cast<std::size_t>(row)]);
        restored = true;
        dialog.accept();
    });
    dialog.exec();
    if (restored)
        captureNow();
    return restored;
}
} // namespace vibmory
