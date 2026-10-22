#pragma once

#include "application/document_store.h"
#include "application/editor_session.h"

namespace vibmory
{
struct SaveOutcome
{
    bool savedCurrentRevision = false;
};

// Owns publication/save-identity policy. The UI owns path selection and
// consent.
class DocumentWorkflow
{
public:
    DocumentWorkflow(EditorSession& session, const DocumentStore& store);
    [[nodiscard]] Result<PatternDocument> read(
        const std::filesystem::path& path) const;
    void replace(PatternDocument document, std::filesystem::path path = {});
    [[nodiscard]] Result<void> open(const std::filesystem::path& path);
    [[nodiscard]] Result<SaveOutcome> save(const std::filesystem::path& path);

private:
    EditorSession& session_;
    const DocumentStore& store_;
};
} // namespace vibmory
