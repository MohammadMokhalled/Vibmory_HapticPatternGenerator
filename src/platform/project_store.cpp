#include "platform/project_store.h"

#include "persistence/project_codec.h"
#include "platform/text_file.h"

#include <QDebug>
#include <QFile>
#include <QLoggingCategory>
#include <QSaveFile>

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

QString toQString(const std::filesystem::path& path)
{
#ifdef Q_OS_WIN
    return QString::fromStdWString(path.native());
#else
    return QString::fromStdString(path.native());
#endif
}

std::filesystem::path toPath(const QString& path)
{
#ifdef Q_OS_WIN
    return std::filesystem::path(path.toStdWString());
#else
    return std::filesystem::path(path.toStdString());
#endif
}

Result<std::string> readProjectText(const std::filesystem::path& path)
{
    return readTextFile(path, maxProjectBytes);
}

Result<PatternDocument> QtProjectStore::load(
    const std::filesystem::path& path) const
{
    auto text = readProjectText(path);
    if (!text)
        return text.error();
    auto document = decodeProject(text.value());
    if (!document)
        return Error{document.error().code, toQString(path).toStdString() +
                                                ": " +
                                                document.error().message};
    return document;
}

Result<PatternDocument> importLegacy(const std::filesystem::path& path,
                                     LegacyOrientation orientation)
{
    auto text = readProjectText(path);
    if (!text)
        return text.error();
    auto document = decodeLegacyCsv(text.value(), orientation);
    if (!document)
        return Error{document.error().code, toQString(path).toStdString() +
                                                ": " +
                                                document.error().message};
    return document;
}

Result<void> QtProjectStore::save(const std::filesystem::path& path,
                                  const PatternDocument& document) const
{
    const auto text = encodeProject(document);
    return writeTextFileAtomically(path, text);
}

Result<void> exportWaveFile(const std::filesystem::path& path,
                            PatternDocument document,
                            const RenderProgress& progress)
{
    QSaveFile file(toQString(path));
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly))
        return fileError(file);
    auto result = renderWave(
        std::move(document),
        [&file](auto bytes) -> Result<void> {
            if (file.write(reinterpret_cast<const char*>(bytes.data()),
                           static_cast<qint64>(bytes.size())) !=
                static_cast<qint64>(bytes.size()))
                return fileError(file);
            return {};
        },
        progress);
    if (!result)
    {
        file.cancelWriting();
        return result;
    }
    if (!file.commit())
        return fileError(file);
    return {};
}
} // namespace vibmory
