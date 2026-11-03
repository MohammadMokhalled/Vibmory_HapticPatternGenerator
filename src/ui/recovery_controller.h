#pragma once

#include "application/editor_session.h"
#include "platform/recovery_store.h"

#include <QFutureWatcher>
#include <QObject>
#include <QTimer>

#include <atomic>
#include <optional>

class QWidget;

namespace vibmory
{
using RecoveryWriteOperation = std::function<Result<void>(
    const std::filesystem::path&, const RecoveryRecord&,
    const std::function<bool()>&)>;

// GUI-thread coordinator. Workers receive only immutable values and a shared
// cancellation flag; the session and file lease remain on the GUI thread.
class RecoveryController final : public QObject
{
    Q_OBJECT
public:
    RecoveryController(EditorSession& session, QString directory,
                       RecoveryWriteOperation write = writeRecoveryFile,
                       QObject* parent = nullptr);
    ~RecoveryController() override;
    void observe();
    void captureNow();
    void clear();
    [[nodiscard]] bool offerRestore(QWidget* parent);
    [[nodiscard]] bool busy() const
    {
        return state_ != nullptr;
    }

signals:
    void snapshotSaved(quint64 revision);
    void warningOccurred(const QString& message);

private:
    struct State
    {
        std::atomic<bool> cancel{false};
        std::uint64_t revision;
    };
    void stopAndWait();
    void report(const Error& error);

    EditorSession& session_;
    QString directory_;
    RecoveryWriteOperation write_;
    QTimer timer_;
    QFutureWatcher<Result<void>> watcher_;
    std::unique_ptr<RecoveryFile> file_;
    std::shared_ptr<State> state_;
    std::optional<std::uint64_t> writtenRevision_;
    std::uint64_t generation_;
};
} // namespace vibmory
