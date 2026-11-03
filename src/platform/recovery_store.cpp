#include "platform/recovery_store.h"

#include "persistence/project_codec.h"
#include "platform/project_store.h"
#include "platform/text_file.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

namespace vibmory
{
namespace
{
constexpr auto signature = "VIBMORY_RECOVERY 1\n";
constexpr std::size_t maxMetadataBytes = 8192;

std::unique_ptr<QLockFile> acquire(const QString& path)
{
    auto lock = std::make_unique<QLockFile>(path + ".lock");
    // Never steal a lock merely because an editing session is long. Qt still
    // detects and reclaims a dead process's lock on the same machine.
    lock->setStaleLockTime(0);
    return lock->tryLock(0) ? std::move(lock) : nullptr;
}
} // namespace

RecoveryFile::RecoveryFile(QString path, std::unique_ptr<QLockFile> lock)
    : path_(std::move(path)), lock_(std::move(lock))
{
}

Result<std::unique_ptr<RecoveryFile>> RecoveryFile::create(
    const QString& directory)
{
    if (directory.isEmpty() || !QDir().mkpath(directory))
        return Error{ErrorCode::Io, "Cannot create the recovery directory."};
    const auto path = QDir(directory).absoluteFilePath(
        QUuid::createUuid().toString(QUuid::WithoutBraces) + ".vibrecovery");
    auto lock = acquire(path);
    if (!lock)
        return Error{ErrorCode::Io, "Cannot lock the recovery file."};
    return std::unique_ptr<RecoveryFile>(
        new RecoveryFile(path, std::move(lock)));
}

Result<void> RecoveryFile::discard()
{
    if (QFile::exists(path_) && !QFile::remove(path_))
        return Error{ErrorCode::Io,
                     "Cannot remove recovery file: " + path_.toStdString()};
    return {};
}

Result<std::vector<std::unique_ptr<RecoveryFile>>> RecoveryFile::available(
    const QString& directory, const QDateTime& now)
{
    if (directory.isEmpty())
        return invalid("A recovery directory is required.");
    QDir folder(directory);
    if (!folder.exists())
        return std::vector<std::unique_ptr<RecoveryFile>>{};
    if (!folder.isReadable())
        return Error{ErrorCode::Io, "Cannot read the recovery directory."};
    // A crash before the first successful snapshot can leave only a lock.
    // Acquire/release it through Qt so live writers remain protected.
    for (const auto& orphan : folder.entryInfoList(
             {"*.vibrecovery.lock"}, QDir::Files | QDir::NoSymLinks))
    {
        auto path = orphan.absoluteFilePath();
        path.chop(5);
        if (!QFile::exists(path) &&
            !QUuid::fromString(QFileInfo(path).completeBaseName()).isNull())
            (void)acquire(path);
    }
    std::vector<std::unique_ptr<RecoveryFile>> result;
    const auto files = folder.entryInfoList(
        {"*.vibrecovery"}, QDir::Files | QDir::NoSymLinks, QDir::Time);
    for (const auto& file : files)
    {
        if (QUuid::fromString(file.completeBaseName()).isNull())
            continue;
        auto lock = acquire(file.absoluteFilePath());
        if (!lock)
            continue;
        auto candidate = std::unique_ptr<RecoveryFile>(
            new RecoveryFile(file.absoluteFilePath(), std::move(lock)));
        // Retain the ten newest inactive sessions for up to thirty days.
        // Locked files belonging to live editors are never pruned.
        if (result.size() >= 10 ||
            file.lastModified().toUTC() < now.addDays(-30))
        {
            if (auto removed = candidate->discard(); !removed)
                return removed.error();
        }
        else
            result.push_back(std::move(candidate));
    }
    return result;
}

Result<void> writeRecoveryFile(const std::filesystem::path& path,
                               const RecoveryRecord& record,
                               const std::function<bool()>& continueWriting)
{
    if (!record.capturedAt.isValid())
        return invalid("Recovery capture time is invalid.");
    const auto original =
        record.originalPath.empty()
            ? QString{}
            : QFileInfo(toQString(record.originalPath)).absoluteFilePath();
    if (original.contains(QChar::Null) || original.toUtf8().size() > 4096)
        return invalid("Recovery project path is invalid or too long.");
    const QJsonObject metadata{
        {"originalPath", original},
        {"capturedAt", record.capturedAt.toUTC().toString(Qt::ISODateWithMs)}};
    auto text =
        std::string(signature) +
        QJsonDocument(metadata).toJson(QJsonDocument::Compact).toStdString() +
        '\n';
    if (text.size() > maxMetadataBytes)
        return invalid("Recovery metadata exceeds its size limit.");
    text += encodeProject(record.document);
    return writeTextFileAtomically(path, text, continueWriting);
}

Result<RecoveryRecord> readRecoveryFile(const std::filesystem::path& path)
{
    const auto text = readTextFile(path, maxProjectBytes + maxMetadataBytes);
    if (!text)
        return text.error();
    const auto failure =
        [&](const std::string& message) -> Result<RecoveryRecord> {
        return Error{ErrorCode::Parse,
                     toQString(path).toStdString() + ": " + message};
    };
    const std::string_view content = text.value();
    if (!content.starts_with(signature))
        return failure("Unsupported recovery signature or version.");
    const auto metadataStart = std::char_traits<char>::length(signature);
    const auto metadataEnd = content.find('\n', metadataStart);
    if (metadataEnd == std::string_view::npos ||
        metadataEnd >= maxMetadataBytes)
        return failure("Missing or oversized recovery metadata.");
    QJsonParseError error;
    const auto parsed = QJsonDocument::fromJson(
        QByteArray(content.data() + metadataStart,
                   static_cast<qsizetype>(metadataEnd - metadataStart)),
        &error);
    const auto object = parsed.object();
    if (error.error != QJsonParseError::NoError || !parsed.isObject() ||
        object.size() != 2 || !object["originalPath"].isString() ||
        !object["capturedAt"].isString())
        return failure("Invalid recovery metadata fields.");
    const auto original = object["originalPath"].toString();
    const auto captured = QDateTime::fromString(object["capturedAt"].toString(),
                                                Qt::ISODateWithMs);
    if (!captured.isValid() || original.contains(QChar::Null) ||
        original.toUtf8().size() > 4096 ||
        (!original.isEmpty() && !QFileInfo(original).isAbsolute()))
        return failure("Invalid recovery project path or capture time.");
    auto document = decodeProject(content.substr(metadataEnd + 1));
    if (!document)
        return failure(document.error().message);
    return RecoveryRecord{std::move(document.value()), toPath(original),
                          captured.toUTC()};
}
} // namespace vibmory
