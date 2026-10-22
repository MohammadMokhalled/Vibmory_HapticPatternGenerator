#pragma once

#include "application/document_store.h"
#include "audio/pcm_renderer.h"
#include "persistence/project_codec.h"

#include <QString>

namespace vibmory
{
[[nodiscard]] QString toQString(const std::filesystem::path &path);
[[nodiscard]] std::filesystem::path toPath(const QString &path);
[[nodiscard]] Result<std::string> readProjectText(
    const std::filesystem::path &path);
[[nodiscard]] Result<PatternDocument> importLegacy(
    const std::filesystem::path &path, LegacyOrientation orientation);

class QtProjectStore final : public DocumentStore
{
        public:
    [[nodiscard]] Result<PatternDocument> load(
        const std::filesystem::path &path) const override;
    [[nodiscard]] Result<void> save(
        const std::filesystem::path &path,
        const PatternDocument &document) const override;
};

// Constructs its file on the calling worker thread. Commit is all-or-nothing.
[[nodiscard]] Result<void> exportWaveFile(const std::filesystem::path &path,
                                          PatternDocument document,
                                          const RenderProgress &progress = {});
} // namespace vibmory
