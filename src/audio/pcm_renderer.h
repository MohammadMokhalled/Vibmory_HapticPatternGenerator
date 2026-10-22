#pragma once

#include "domain/pattern.h"

#include <functional>
#include <span>

namespace vibmory
{
class PcmRenderer
{
        public:
    [[nodiscard]] static Result<PcmRenderer> create(PatternDocument document);
    // Output must hold a whole number of interleaved channel frames.
    [[nodiscard]] Result<std::size_t> render(std::span<std::int16_t> output);
    [[nodiscard]] std::uint64_t position() const
    {
        return position_;
    }
    [[nodiscard]] std::uint64_t totalFrames() const
    {
        return document_.settings().sampleFrames();
    }
    [[nodiscard]] std::size_t channelCount() const
    {
        return phases_.size();
    }
    [[nodiscard]] const PatternDocument &document() const
    {
        return document_;
    }

        private:
    explicit PcmRenderer(PatternDocument document);
    PatternDocument document_;
    std::vector<double> phases_;
    std::uint64_t position_ = 0;
};

using ByteSink = std::function<Result<void>(std::span<const std::byte>)>;
// Return false to cancel. Called on the rendering thread, never the GUI thread.
using RenderProgress =
    std::function<bool(std::uint64_t completed, std::uint64_t total)>;

[[nodiscard]] Result<std::vector<std::byte>> waveHeader(
    const PatternDocument &document);
[[nodiscard]] Result<void> renderWave(PatternDocument document,
                                      const ByteSink &sink,
                                      const RenderProgress &progress = {});
[[nodiscard]] std::vector<std::byte> littleEndianPcm(
    std::span<const std::int16_t> samples);
} // namespace vibmory
