#include "application/document_workflow.h"

#include <exception>

namespace vibmory
{
DocumentWorkflow::DocumentWorkflow(EditorSession& session,
                                   const DocumentStore& store)
    : session_(session), store_(store)
{
}

Result<PatternDocument> DocumentWorkflow::read(
    const std::filesystem::path& path) const
{
    try
    {
        return store_.load(path);
    }
    catch (const std::exception& error)
    {
        return Error{ErrorCode::Io,
                     std::string("Project load failed: ") + error.what()};
    }
}

void DocumentWorkflow::replace(PatternDocument document,
                               std::filesystem::path path)
{
    session_.replace(std::move(document), std::move(path));
}

Result<void> DocumentWorkflow::open(const std::filesystem::path& path)
{
    auto document = read(path);
    if (!document)
        return document.error();
    replace(std::move(document.value()), path);
    return {};
}

Result<SaveOutcome> DocumentWorkflow::save(const std::filesystem::path& path)
{
    if (path.empty())
        return invalid("Choose a project path before saving.");
    try
    {
        const auto snapshot = session_.snapshot();
        if (auto result = store_.save(path, snapshot.document); !result)
            return result.error();
        const bool current = snapshot.revision == session_.revision();
        if (current)
            session_.markSaved(path);
        return SaveOutcome{current};
    }
    catch (const std::exception& error)
    {
        return Error{ErrorCode::Io,
                     std::string("Project save failed: ") + error.what()};
    }
}
} // namespace vibmory
