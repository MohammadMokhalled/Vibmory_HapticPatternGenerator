#include "audio/pcm_renderer.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <string_view>

namespace vibmory
{
namespace
{
constexpr double tau = 2 * std::numbers::pi;

void append16(std::vector<std::byte> &bytes, std::uint16_t value)
{
    bytes.push_back(static_cast<std::byte>(value & 255));
    bytes.push_back(static_cast<std::byte>((value >> 8) & 255));
}

void append32(std::vector<std::byte> &bytes, std::uint32_t value)
{
    append16(bytes, static_cast<std::uint16_t>(value & 65535));
    append16(bytes, static_cast<std::uint16_t>(value >> 16));
}

void fourcc(std::vector<std::byte> &bytes, std::string_view value)
{
    for(const char character : value)
        bytes.push_back(static_cast<std::byte>(character));
}
} // namespace

PcmRenderer::PcmRenderer(PatternDocument document)
    : document_(std::move(document)), phases_(document_.shape().cellCount(), 0)
{
}

Result<PcmRenderer> PcmRenderer::create(PatternDocument document)
{
    if(auto result = document.validateOutput(); !result)
        return result.error();
    return PcmRenderer(std::move(document));
}

Result<std::size_t> PcmRenderer::render(std::span<std::int16_t> output)
{
    if(output.empty() || output.size() % channelCount() != 0)
        return invalid(
            "PCM output must contain complete interleaved channel frames.");
    const auto frames = static_cast<std::size_t>(std::min<std::uint64_t>(
        output.size() / channelCount(), totalFrames() - position_));
    const auto &settings = document_.settings();
    const auto &routing = document_.routing();
    for(std::size_t sample = 0; sample < frames; ++sample, ++position_)
    {
        const auto frameIndex = static_cast<std::size_t>(
            (position_ * settings.frameRate / settings.sampleRate) %
            document_.frameCount());
        const auto &values = document_.frame(frameIndex);
        for(std::size_t channel = 0; channel < channelCount(); ++channel)
        {
            const auto stimulus = values[routing.cells[channel]];
            const double phase = settings.phase == PhasePolicy::Continuous
                                     ? phases_[channel]
                                     : (tau * stimulus.frequencyHz) *
                                           (static_cast<double>(position_) /
                                            settings.sampleRate);
            const double scaled =
                stimulus.amplitude * routing.gains[channel] * std::sin(phase);
            const auto value = settings.phase == PhasePolicy::LegacyAbsolute
                                   ? static_cast<long>(scaled)
                                   : std::lround(scaled);
            output[sample * channelCount() + channel] =
                static_cast<std::int16_t>(std::clamp(value, -32767L, 32767L));
            phases_[channel] =
                std::fmod(phases_[channel] +
                              tau * stimulus.frequencyHz / settings.sampleRate,
                          tau);
        }
    }
    return frames;
}

std::vector<std::byte> littleEndianPcm(std::span<const std::int16_t> samples)
{
    std::vector<std::byte> bytes;
    bytes.reserve(samples.size() * 2);
    for(const auto sample : samples)
        append16(bytes, static_cast<std::uint16_t>(sample));
    return bytes;
}

Result<std::vector<std::byte>> waveHeader(const PatternDocument &document)
{
    if(auto result = document.validateOutput(); !result)
        return result.error();
    const auto channels =
        static_cast<std::uint16_t>(document.shape().cellCount());
    const bool extended = channels > 2;
    const auto dataBytes = static_cast<std::uint32_t>(
        document.settings().sampleFrames() * channels * 2);
    const std::uint32_t headerSize = extended ? 68U : 44U;
    std::vector<std::byte> bytes;
    fourcc(bytes, "RIFF");
    append32(bytes, dataBytes + headerSize - 8);
    fourcc(bytes, "WAVEfmt ");
    append32(bytes, extended ? 40U : 16U);
    append16(bytes, extended ? 0xFFFE : 1);
    append16(bytes, channels);
    append32(bytes, document.settings().sampleRate);
    append32(bytes, document.settings().sampleRate * channels * 2);
    append16(bytes, static_cast<std::uint16_t>(channels * 2));
    append16(bytes, 16);
    if(extended)
    {
        append16(bytes, 22); // WAVEFORMATEXTENSIBLE extension length.
        append16(bytes, 16); // Valid bits per sample.
        append32(bytes,
                 0); // Unassigned physical channels, not speaker remapping.
        append32(bytes, 1); // KSDATAFORMAT_SUBTYPE_PCM GUID.
        append16(bytes, 0);
        append16(bytes, 0x0010);
        for(const auto value : {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71})
            bytes.push_back(static_cast<std::byte>(value));
    }
    fourcc(bytes, "data");
    append32(bytes, dataBytes);
    return bytes;
}

Result<void> renderWave(PatternDocument document, const ByteSink &sink,
                        const RenderProgress &progress)
{
    if(!sink)
        return invalid("An output sink is required.");
    auto header = waveHeader(document);
    if(!header)
        return header.error();
    auto rendererResult = PcmRenderer::create(std::move(document));
    if(!rendererResult)
        return rendererResult.error();
    auto renderer = std::move(rendererResult.value());
    if(progress && !progress(0, renderer.totalFrames()))
        return Error{ErrorCode::Cancelled, "Audio rendering cancelled."};
    if(auto result = sink(header.value()); !result)
        return result;
    std::vector<std::int16_t> block(4096 * renderer.channelCount());
    while(renderer.position() < renderer.totalFrames())
    {
        auto count = renderer.render(block);
        if(!count)
            return count.error();
        const auto bytes = littleEndianPcm(
            std::span(block).first(count.value() * renderer.channelCount()));
        if(auto result = sink(bytes); !result)
            return result;
        if(progress && !progress(renderer.position(), renderer.totalFrames()))
            return Error{ErrorCode::Cancelled, "Audio rendering cancelled."};
    }
    return {};
}
} // namespace vibmory
