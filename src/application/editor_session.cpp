#include "application/editor_session.h"

#include <algorithm>

namespace vibmory
{
EditorSession::EditorSession(PatternDocument document)
    : state_{std::move(document), 0, {}, 0}
{
}

Result<void> EditorSession::selectFrame(std::size_t frame)
{
    if(frame >= document().frameCount())
        return invalid("Cannot select an unknown frame.");
    endEditGroup();
    state_.frame = frame;
    state_.cell.reset();
    return {};
}

Result<void> EditorSession::selectCell(std::optional<CellPosition> cell)
{
    if(cell && !document().cell(state_.frame, *cell))
        return invalid("Cannot select a cell outside the grid.");
    endEditGroup();
    state_.cell = cell;
    return {};
}

Result<void> EditorSession::change(
    const std::function<Result<void>(State &)> &command)
{
    auto candidate = state_;
    if(auto result = command(candidate); !result)
        return result;
    if(candidate.document == state_.document)
        return {};
    if(!grouping_ || !groupRecorded_)
        undo_.push_back(state_);
    groupRecorded_ = grouping_;
    redo_.clear();
    candidate.id = nextState_++;
    state_ = std::move(candidate);
    ++revision_;
    trimHistory();
    return {};
}

void EditorSession::trimHistory()
{
    constexpr std::size_t budget = 32 * 1024 * 1024;
    auto bytes = [](const State &state) {
        return state.document.shape().cellCount() *
               state.document.frameCount() * sizeof(CellStimulus);
    };
    std::size_t total = 0;
    for(const auto &state : undo_)
        total += bytes(state);
    while(undo_.size() > 32 || (total > budget && undo_.size() > 1))
    {
        total -= bytes(undo_.front());
        undo_.erase(undo_.begin());
    }
}

Result<void> EditorSession::editCell(CellStimulus value)
{
    if(!state_.cell)
        return invalid("Select a cell before editing its stimulus.");
    return change([value](State &state) {
        return state.document.setCell(state.frame, *state.cell, value);
    });
}

Result<void> EditorSession::setSettings(PlaybackSettings settings)
{
    endEditGroup();
    return change([settings](State &state) {
        return state.document.setSettings(settings);
    });
}

Result<void> EditorSession::setRouting(ChannelRouting routing)
{
    endEditGroup();
    return change([&routing](State &state) {
        return state.document.setRouting(routing);
    });
}

Result<void> EditorSession::addFrame()
{
    endEditGroup();
    return change([](State &state) -> Result<void> {
        if(auto result = state.document.addFrame(); !result)
            return result;
        state.frame = state.document.frameCount() - 1;
        state.cell.reset();
        return {};
    });
}

Result<void> EditorSession::duplicateFrame()
{
    endEditGroup();
    return change([](State &state) -> Result<void> {
        if(auto result = state.document.duplicateFrame(state.frame); !result)
            return result;
        ++state.frame;
        state.cell.reset();
        return {};
    });
}

Result<void> EditorSession::removeFrame()
{
    endEditGroup();
    return change([](State &state) -> Result<void> {
        if(auto result = state.document.removeFrame(state.frame); !result)
            return result;
        state.frame = std::min(state.frame, state.document.frameCount() - 1);
        state.cell.reset();
        return {};
    });
}

Result<void> EditorSession::moveFrame(std::size_t from, std::size_t to)
{
    endEditGroup();
    return change([from, to](State &state) -> Result<void> {
        if(auto result = state.document.moveFrame(from, to); !result)
            return result;
        if(state.frame == from)
            state.frame = to;
        else if(from < state.frame && state.frame <= to)
            --state.frame;
        else if(to <= state.frame && state.frame < from)
            ++state.frame;
        return {};
    });
}

void EditorSession::beginEditGroup()
{
    grouping_ = true;
    groupRecorded_ = false;
}
void EditorSession::endEditGroup()
{
    grouping_ = false;
    groupRecorded_ = false;
}

void EditorSession::undo()
{
    endEditGroup();
    if(undo_.empty())
        return;
    redo_.push_back(state_);
    state_ = std::move(undo_.back());
    undo_.pop_back();
    ++revision_;
}

void EditorSession::redo()
{
    endEditGroup();
    if(redo_.empty())
        return;
    undo_.push_back(state_);
    state_ = std::move(redo_.back());
    redo_.pop_back();
    ++revision_;
    trimHistory();
}

void EditorSession::markSaved(std::filesystem::path path)
{
    endEditGroup();
    path_ = std::move(path);
    savedState_ = state_.id;
}

void EditorSession::replace(PatternDocument document,
                            std::filesystem::path path)
{
    endEditGroup();
    state_ = {std::move(document), 0, {}, nextState_++};
    undo_.clear();
    redo_.clear();
    savedState_ = state_.id;
    path_ = std::move(path);
    ++revision_;
}
} // namespace vibmory
