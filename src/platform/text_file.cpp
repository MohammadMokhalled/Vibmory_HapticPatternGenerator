#include "platform/text_file.h"

#include "platform/project_store.h"

#include <QDebug>
#include <QFile>
#include <QLoggingCategory>
#include <QSaveFile>

#include <limits>

namespace vibmory
{
namespace
{
Q_LOGGING_CATEGORY(storageLog, "vibmory.storage")
Error fileError(const QFileDevice& file)
{
    qCWarning(storageLog).noquote() << file.fileName() << file.errorString();
    return {ErrorCode::Io,
            (file.fileName() + ": " + file.errorString()).toStdString()};
}
} // namespace

Result<std::string> readTextFile(const std::filesystem::path& path,
                                 std::size_t maximumBytes)
{
    if (maximumBytes >=
        static_cast<std::size_t>(std::numeric_limits<qint64>::max()))
        return invalid("Invalid text file size limit.");
    QFile file(toQString(path));
    if (!file.open(QIODevice::ReadOnly))
        return fileError(file);
    const auto limit = static_cast<qint64>(maximumBytes);
    if (file.size() < 0 || file.size() > limit)
        return invalid(toQString(path).toStdString() + ": file exceeds the " +
                       std::to_string(maximumBytes) + " byte input limit.");
    const auto data = file.read(limit + 1);
    if (file.error() != QFileDevice::NoError)
        return fileError(file);
    if (data.size() > limit)
        return invalid(toQString(path).toStdString() +
                       ": file grew beyond its input limit.");
    return data.toStdString();
}

Result<void> writeTextFileAtomically(const std::filesystem::path& path,
                                     std::string_view text)
{
    if (text.size() >
        static_cast<std::size_t>(std::numeric_limits<qint64>::max()))
        return invalid("Text exceeds the file write limit.");
    QSaveFile file(toQString(path));
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly))
        return fileError(file);
    const auto size = static_cast<qint64>(text.size());
    if (file.write(text.data(), size) != size || !file.commit())
        return fileError(file);
    return {};
}
} // namespace vibmory
