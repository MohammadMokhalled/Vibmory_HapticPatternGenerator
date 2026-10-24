#pragma once

#include "domain/pattern.h"

#include <QDateTime>
#include <QLockFile>
#include <QString>

#include <filesystem>
#include <functional>
#include <memory>

namespace vibmory
{
struct RecoveryRecord
{
    PatternDocument document;
    std::filesystem::path originalPath;
    QDateTime capturedAt;
};

[[nodiscard]] Result<void> writeRecoveryFile(
    const std::filesystem::path& path, const RecoveryRecord& record,
    const std::function<bool()>& continueWriting);
[[nodiscard]] Result<RecoveryRecord> readRecoveryFile(
    const std::filesystem::path& path);

// A lease protects one recovery file from other running instances. Destruction
// releases the lock but preserves the file; only explicit discard/clean close
// removes it. A crashed process leaves a stale lock that QLockFile can reclaim.
class RecoveryFile
{
public:
    [[nodiscard]] static Result<std::unique_ptr<RecoveryFile>> create(
        const QString& directory);
    [[nodiscard]] static Result<std::vector<std::unique_ptr<RecoveryFile>>>
    available(const QString& directory,
              const QDateTime& now = QDateTime::currentDateTimeUtc());
    [[nodiscard]] QString path() const
    {
        return path_;
    }
    [[nodiscard]] Result<void> discard();

private:
    RecoveryFile(QString path, std::unique_ptr<QLockFile> lock);
    QString path_;
    std::unique_ptr<QLockFile> lock_;
};
} // namespace vibmory
