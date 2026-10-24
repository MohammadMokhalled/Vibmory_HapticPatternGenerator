#pragma once

#include "domain/pattern.h"

#include <filesystem>
#include <functional>
#include <optional>

namespace vibmory
{
class EditorSession
{
public:
    explicit EditorSession(PatternDocument document);
    [[nodiscard]] const PatternDocument& document() const
    {
        return state_.document;
    }
    [[nodiscard]] std::size_t selectedFrame() const
    {
        return state_.frame;
    }
    [[nodiscard]] std::optional<CellPosition> selectedCell() const
    {
        return state_.cell;
    }
    [[nodiscard]] std::uint64_t revision() const
    {
        return revision_;
    }
    [[nodiscard]] std::uint64_t documentGeneration() const
    {
        return documentGeneration_;
    }
    [[nodiscard]] bool dirty() const
    {
        return state_.id != savedState_;
    }
    [[nodiscard]] const std::filesystem::path& path() const
    {
        return path_;
    }
    [[nodiscard]] DocumentSnapshot snapshot() const
    {
        return {state_.document, revision_};
    }
    [[nodiscard]] Result<void> selectFrame(std::size_t frame);
    [[nodiscard]] Result<void> selectCell(std::optional<CellPosition> cell);
    [[nodiscard]] Result<void> editCell(CellStimulus value);
    [[nodiscard]] Result<void> setSettings(PlaybackSettings settings);
    [[nodiscard]] Result<void> setRouting(ChannelRouting routing);
    [[nodiscard]] Result<void> addFrame();
    [[nodiscard]] Result<void> duplicateFrame();
    [[nodiscard]] Result<void> removeFrame();
    [[nodiscard]] Result<void> moveFrame(std::size_t from, std::size_t to);
    void beginEditGroup();
    void endEditGroup();
    [[nodiscard]] bool canUndo() const
    {
        return !undo_.empty();
    }
    [[nodiscard]] bool canRedo() const
    {
        return !redo_.empty();
    }
    void undo();
    void redo();
    void markSaved(std::filesystem::path path);
    void replace(PatternDocument document, std::filesystem::path path = {});
    void restore(PatternDocument document,
                 std::filesystem::path originalPath = {});

private:
    struct State
    {
        PatternDocument document;
        std::size_t frame = 0;
        std::optional<CellPosition> cell;
        std::uint64_t id = 0;
    };
    [[nodiscard]] Result<void> change(
        const std::function<Result<void>(State&)>& command);
    void trimHistory();
    State state_;
    std::vector<State> undo_;
    std::vector<State> redo_;
    std::uint64_t revision_ = 0;
    std::uint64_t documentGeneration_ = 0;
    std::uint64_t nextState_ = 1;
    std::uint64_t savedState_ = 0;
    std::filesystem::path path_;
    bool grouping_ = false;
    bool groupRecorded_ = false;
};
} // namespace vibmory
