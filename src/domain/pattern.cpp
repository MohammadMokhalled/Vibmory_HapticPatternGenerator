#include "domain/pattern.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace vibmory
{
std::size_t GridShape::cellCount() const
{
    return static_cast<std::size_t>(rows) * columns;
}

Result<void> GridShape::validate() const
{
    if(rows == 0 || columns == 0 || rows > maxCells || columns > maxCells ||
       cellCount() > maxCells)
        return invalid(
            "Grid dimensions must be positive with at most 256 cells.");
    return {};
}

Result<void> PlaybackSettings::validate() const
{
    if(frameRate == 0 || frameRate > 240)
        return invalid("Frame rate must be between 1 and 240 frames/second.");
    if(durationMs == 0 || durationMs > 600000)
        return invalid("Duration must be between 1 ms and 10 minutes.");
    if(sampleRate < 8000 || sampleRate > 192000)
        return invalid("Sample rate must be between 8000 and 192000 Hz.");
    if(phase != PhasePolicy::Continuous && phase != PhasePolicy::LegacyAbsolute)
        return invalid("Unknown oscillator phase policy.");
    return {};
}

std::uint64_t PlaybackSettings::sampleFrames() const
{
    return static_cast<std::uint64_t>(sampleRate) * durationMs / 1000;
}

ChannelRouting ChannelRouting::identity(std::size_t count)
{
    ChannelRouting result{std::vector<std::size_t>(count),
                          std::vector<double>(count, 1.0)};
    std::iota(result.cells.begin(), result.cells.end(), std::size_t{0});
    return result;
}

Result<void> ChannelRouting::validate(std::size_t count) const
{
    if(cells.size() != count || gains.size() != count)
        return invalid("Channel routing and gain counts must match the grid.");
    std::vector<bool> seen(count, false);
    for(std::size_t i = 0; i < count; ++i)
    {
        if(cells[i] >= count || seen[cells[i]])
            return invalid(
                "Channel routing must be a permutation of logical cells.");
        if(!std::isfinite(gains[i]) || gains[i] < 0 || gains[i] > 1)
            return invalid(
                "Channel gains must be finite values between 0 and 1.");
        seen[cells[i]] = true;
    }
    return {};
}

PatternDocument::PatternDocument(GridShape shape)
    : shape_(shape), frames_(1, std::vector<CellStimulus>(shape.cellCount())),
      routing_(ChannelRouting::identity(shape.cellCount()))
{
}

Result<PatternDocument> PatternDocument::create(GridShape shape)
{
    if(auto result = shape.validate(); !result)
        return result.error();
    return PatternDocument(shape);
}

bool PatternDocument::contains(std::size_t frameIndex, CellPosition pos) const
{
    return frameIndex < frames_.size() && pos.row < shape_.rows &&
           pos.column < shape_.columns;
}

Result<CellStimulus> PatternDocument::cell(std::size_t frameIndex,
                                           CellPosition pos) const
{
    if(!contains(frameIndex, pos))
        return invalid("Frame or cell index is outside the document.");
    return frames_[frameIndex]
                  [static_cast<std::size_t>(pos.row) * shape_.columns +
                   pos.column];
}

const std::vector<CellStimulus> &PatternDocument::frame(std::size_t index) const
{
    return frames_.at(index);
}

Result<void> PatternDocument::setCell(std::size_t frameIndex, CellPosition pos,
                                      CellStimulus value)
{
    if(!contains(frameIndex, pos))
        return invalid("Frame or cell index is outside the document.");
    if(value.amplitude > maxAmplitude || value.frequencyHz > maxFrequencyHz ||
       value.frequencyHz * 2 >= settings_.sampleRate)
        return invalid("Cell values exceed the amplitude, frequency, or "
                       "sample-rate limits.");
    frames_[frameIndex][static_cast<std::size_t>(pos.row) * shape_.columns +
                        pos.column] = value;
    return {};
}

Result<void> PatternDocument::setSettings(PlaybackSettings settings)
{
    if(auto result = settings.validate(); !result)
        return result;
    for(const auto &frameValues : frames_)
        for(const auto &value : frameValues)
            if(value.frequencyHz * 2 >= settings.sampleRate)
                return invalid("A cell frequency is too high for the requested "
                               "sample rate.");
    if(settings.sampleFrames() * shape_.cellCount() * 2 > maxOutputBytes)
        return invalid(
            "Rendered audio would exceed the 512 MiB output budget.");
    settings_ = settings;
    return {};
}

Result<void> PatternDocument::setRouting(ChannelRouting routing)
{
    if(auto result = routing.validate(shape_.cellCount()); !result)
        return result;
    routing_ = std::move(routing);
    return {};
}

Result<void> PatternDocument::canInsertFrame() const
{
    if(frames_.size() >= maxFrames ||
       (frames_.size() + 1) * shape_.cellCount() > maxDocumentCells)
        return invalid("The document's frame/cell budget would be exceeded.");
    return {};
}

Result<void> PatternDocument::addFrame()
{
    if(auto result = canInsertFrame(); !result)
        return result;
    frames_.emplace_back(shape_.cellCount());
    return {};
}

Result<void> PatternDocument::duplicateFrame(std::size_t index)
{
    if(index >= frames_.size())
        return invalid("Cannot duplicate an unknown frame.");
    if(auto result = canInsertFrame(); !result)
        return result;
    const auto copy = frames_[index];
    frames_.insert(frames_.begin() + static_cast<std::ptrdiff_t>(index + 1),
                   copy);
    return {};
}

Result<void> PatternDocument::removeFrame(std::size_t index)
{
    if(index >= frames_.size() || frames_.size() == 1)
        return invalid("Cannot remove this frame; a document must contain at "
                       "least one frame.");
    frames_.erase(frames_.begin() + static_cast<std::ptrdiff_t>(index));
    return {};
}

Result<void> PatternDocument::moveFrame(std::size_t from, std::size_t to)
{
    if(from >= frames_.size() || to >= frames_.size())
        return invalid("Cannot move an unknown frame.");
    if(from < to)
        std::rotate(frames_.begin() + static_cast<std::ptrdiff_t>(from),
                    frames_.begin() + static_cast<std::ptrdiff_t>(from + 1),
                    frames_.begin() + static_cast<std::ptrdiff_t>(to + 1));
    else if(from > to)
        std::rotate(frames_.begin() + static_cast<std::ptrdiff_t>(to),
                    frames_.begin() + static_cast<std::ptrdiff_t>(from),
                    frames_.begin() + static_cast<std::ptrdiff_t>(from + 1));
    return {};
}

Result<void> PatternDocument::validateOutput() const
{
    if(auto result = settings_.validate(); !result)
        return result;
    if(settings_.sampleFrames() * shape_.cellCount() * 2 > maxOutputBytes)
        return invalid(
            "Rendered audio would exceed the 512 MiB output budget.");
    return routing_.validate(shape_.cellCount());
}
} // namespace vibmory
