#pragma once

#include "platform/project_store.h"

#include <QFutureWatcher>
#include <QObject>
#include <QTimer>

#include <atomic>
#include <memory>

namespace vibmory
{
using ExportOperation = std::function<Result<void>(
    const std::filesystem::path&, PatternDocument, const RenderProgress&)>;

// GUI-thread coordinator; worker state contains no QObject or editor
// references.
class WaveExportJob final : public QObject
{
    Q_OBJECT
public:
    explicit WaveExportJob(ExportOperation operation = exportWaveFile,
                           QObject* parent = nullptr);
    ~WaveExportJob() override;
    [[nodiscard]] Result<void> start(std::filesystem::path path,
                                     DocumentSnapshot snapshot);
    [[nodiscard]] bool busy() const
    {
        return state_ != nullptr;
    }
    void cancel() noexcept;
    void cancelAndWait();

signals:
    void progressChanged(int percent);
    void finished(const vibmory::Result<void>& result);

private:
    struct State
    {
        std::atomic<bool> cancel{false};
        std::atomic<std::uint64_t> completed{0};
        std::atomic<std::uint64_t> total{1};
    };
    ExportOperation operation_;
    QFutureWatcher<Result<void>> watcher_;
    QTimer progressTimer_;
    std::shared_ptr<State> state_;
};
} // namespace vibmory

Q_DECLARE_METATYPE(vibmory::Result<void>)
