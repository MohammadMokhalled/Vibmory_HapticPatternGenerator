#include "platform/wave_export_job.h"

#include <QDebug>
#include <QLoggingCategory>
#include <QtConcurrentRun>

#include <exception>

namespace vibmory
{
namespace
{
Q_LOGGING_CATEGORY(exportLog, "vibmory.export")
}

WaveExportJob::WaveExportJob(ExportOperation operation, QObject* parent)
    : QObject(parent), operation_(std::move(operation))
{
    qRegisterMetaType<vibmory::Result<void>>();
    progressTimer_.setInterval(25);
    connect(&progressTimer_, &QTimer::timeout, this, [this] {
        if (state_)
            emit progressChanged(static_cast<int>(state_->completed.load() *
                                                  100 / state_->total.load()));
    });
    connect(&watcher_, &QFutureWatcher<Result<void>>::finished, this, [this] {
        progressTimer_.stop();
        const auto result = watcher_.result();
        state_.reset();
        emit finished(result);
    });
}

WaveExportJob::~WaveExportJob()
{
    cancelAndWait();
}

Result<void> WaveExportJob::start(std::filesystem::path path,
                                  DocumentSnapshot snapshot)
{
    if (busy())
        return Error{ErrorCode::Busy, "An audio export is already active."};
    if (!operation_)
        return invalid("An export operation is required.");
    if (auto result = snapshot.document.validateOutput(); !result)
        return result;
    state_ = std::make_shared<State>();
    const auto state = state_;
    watcher_.setFuture(QtConcurrent::run(
        [state, operation = operation_, path = std::move(path),
         document = std::move(snapshot.document)]() mutable -> Result<void> {
            try
            {
                return operation(path, std::move(document),
                                 [state](auto completed, auto total) {
                                     state->completed = completed;
                                     state->total = total;
                                     return !state->cancel.load();
                                 });
            }
            catch (const std::exception& error)
            {
                qCWarning(exportLog) << "Worker exception:" << error.what();
                return Error{ErrorCode::Io,
                             std::string("Audio export failed: ") +
                                 error.what()};
            }
            catch (...)
            {
                qCWarning(exportLog) << "Unknown worker exception";
                return Error{ErrorCode::Io,
                             "Audio export failed unexpectedly."};
            }
        }));
    progressTimer_.start();
    return {};
}

void WaveExportJob::cancel() noexcept
{
    if (state_)
        state_->cancel = true;
}

void WaveExportJob::cancelAndWait()
{
    progressTimer_.stop();
    cancel();
    watcher_.waitForFinished();
    state_.reset();
}
} // namespace vibmory
