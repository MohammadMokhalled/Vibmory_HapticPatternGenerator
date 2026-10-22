#include "platform/project_store.h"

#include "persistence/project_codec.h"

#include <QFile>
#include <QSaveFile>

namespace vibmory
{
namespace
{
Error fileError(const QFileDevice &file)
{
    return {ErrorCode::Io,
            (file.fileName() + ": " + file.errorString()).toStdString()};
}
} // namespace

QString toQString(const std::filesystem::path &path)
{
#ifdef Q_OS_WIN
    return QString::fromStdWString(path.native());
#else
    return QString::fromStdString(path.native());
#endif
}

std::filesystem::path toPath(const QString &path)
{
#ifdef Q_OS_WIN
    return std::filesystem::path(path.toStdWString());
#else
    return std::filesystem::path(path.toStdString());
#endif
}

Result<std::string> readProjectText(const std::filesystem::path &path)
{
    QFile file(toQString(path));
    if(!file.open(QIODevice::ReadOnly))
        return fileError(file);
    if(file.size() < 0 || file.size() > static_cast<qint64>(maxProjectBytes))
        return Error{ErrorCode::Validation,
                     "Project exceeds the 16 MiB input limit."};
    const auto data = file.read(static_cast<qint64>(maxProjectBytes) + 1);
    if(file.error() != QFileDevice::NoError)
        return fileError(file);
    if(data.size() > static_cast<qsizetype>(maxProjectBytes))
        return Error{ErrorCode::Validation,
                     "Project exceeds the 16 MiB input limit."};
    return data.toStdString();
}

Result<PatternDocument> QtProjectStore::load(
    const std::filesystem::path &path) const
{
    auto text = readProjectText(path);
    if(!text)
        return text.error();
    auto document = decodeProject(text.value());
    if(!document)
        return Error{document.error().code, toQString(path).toStdString() +
                                                ": " +
                                                document.error().message};
    return document;
}

Result<PatternDocument> importLegacy(const std::filesystem::path &path,
                                     LegacyOrientation orientation)
{
    auto text = readProjectText(path);
    if(!text)
        return text.error();
    return decodeLegacyCsv(text.value(), orientation);
}

Result<void> QtProjectStore::save(const std::filesystem::path &path,
                                  const PatternDocument &document) const
{
    QSaveFile file(toQString(path));
    file.setDirectWriteFallback(false);
    if(!file.open(QIODevice::WriteOnly))
        return fileError(file);
    const auto text = encodeProject(document);
    if(file.write(text.data(), static_cast<qint64>(text.size())) !=
       static_cast<qint64>(text.size()))
        return fileError(file);
    if(!file.commit())
        return fileError(file);
    return {};
}

Result<void> exportWaveFile(const std::filesystem::path &path,
                            PatternDocument document,
                            const RenderProgress &progress)
{
    QSaveFile file(toQString(path));
    file.setDirectWriteFallback(false);
    if(!file.open(QIODevice::WriteOnly))
        return fileError(file);
    auto result = renderWave(
        std::move(document),
        [&file](auto bytes) -> Result<void> {
            if(file.write(reinterpret_cast<const char *>(bytes.data()),
                          static_cast<qint64>(bytes.size())) !=
               static_cast<qint64>(bytes.size()))
                return fileError(file);
            return {};
        },
        progress);
    if(!result)
    {
        file.cancelWriting();
        return result;
    }
    if(!file.commit())
        return fileError(file);
    return {};
}
} // namespace vibmory
