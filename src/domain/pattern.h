#pragma once

#include "domain/result.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vibmory
{
inline constexpr std::size_t maxCells = 256;
inline constexpr std::size_t maxFrames = 1024;
inline constexpr std::size_t maxDocumentCells = 262144;
inline constexpr std::uint64_t maxOutputBytes = 512ULL * 1024 * 1024;
inline constexpr std::uint32_t maxFrequencyHz = 22000;
inline constexpr std::uint32_t maxAmplitude = 32767;

struct GridShape
{
    std::uint32_t rows = 2;
    std::uint32_t columns = 2;
    [[nodiscard]] std::size_t cellCount() const;
    [[nodiscard]] Result<void> validate() const;
    bool operator==(const GridShape &) const = default;
};

struct CellPosition
{
    std::uint32_t row = 0;
    std::uint32_t column = 0;
    bool operator==(const CellPosition &) const = default;
};

struct CellStimulus
{
    std::uint32_t frequencyHz = 0;
    std::uint32_t amplitude = 0;
    bool operator==(const CellStimulus &) const = default;
};

enum class PhasePolicy
{
    Continuous,
    LegacyAbsolute
};

struct PlaybackSettings
{
    std::uint32_t frameRate = 10;
    std::uint32_t durationMs = 5000;
    std::uint32_t sampleRate = 48000;
    PhasePolicy phase = PhasePolicy::Continuous;
    [[nodiscard]] Result<void> validate() const;
    [[nodiscard]] std::uint64_t sampleFrames() const;
    bool operator==(const PlaybackSettings &) const = default;
};

// The permutation maps physical channel -> row-major logical cell index.
struct ChannelRouting
{
    std::vector<std::size_t> cells;
    std::vector<double> gains;
    [[nodiscard]] static ChannelRouting identity(std::size_t count);
    [[nodiscard]] Result<void> validate(std::size_t count) const;
    bool operator==(const ChannelRouting &) const = default;
};

class PatternDocument
{
        public:
    [[nodiscard]] static Result<PatternDocument> create(GridShape shape);
    [[nodiscard]] GridShape shape() const
    {
        return shape_;
    }
    [[nodiscard]] std::size_t frameCount() const
    {
        return frames_.size();
    }
    [[nodiscard]] const PlaybackSettings &settings() const
    {
        return settings_;
    }
    [[nodiscard]] const ChannelRouting &routing() const
    {
        return routing_;
    }
    [[nodiscard]] Result<CellStimulus> cell(std::size_t frame,
                                            CellPosition pos) const;
    [[nodiscard]] const std::vector<CellStimulus> &frame(
        std::size_t index) const;
    [[nodiscard]] Result<void> setCell(std::size_t frame, CellPosition pos,
                                       CellStimulus value);
    [[nodiscard]] Result<void> setSettings(PlaybackSettings settings);
    [[nodiscard]] Result<void> setRouting(ChannelRouting routing);
    [[nodiscard]] Result<void> addFrame();
    [[nodiscard]] Result<void> duplicateFrame(std::size_t index);
    [[nodiscard]] Result<void> removeFrame(std::size_t index);
    [[nodiscard]] Result<void> moveFrame(std::size_t from, std::size_t to);
    [[nodiscard]] Result<void> validateOutput() const;
    bool operator==(const PatternDocument &) const = default;

        private:
    explicit PatternDocument(GridShape shape);
    [[nodiscard]] bool contains(std::size_t frame, CellPosition pos) const;
    [[nodiscard]] Result<void> canInsertFrame() const;

    GridShape shape_;
    std::vector<std::vector<CellStimulus>> frames_;
    PlaybackSettings settings_;
    ChannelRouting routing_;
};

struct DocumentSnapshot
{
    PatternDocument document;
    std::uint64_t revision = 0;
};
} // namespace vibmory
