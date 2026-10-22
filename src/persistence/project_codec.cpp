#include "persistence/project_codec.h"

#include <charconv>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

namespace vibmory
{
namespace
{
Error parseError(std::string message)
{
    return {ErrorCode::Parse, std::move(message)};
}

std::string_view trim(std::string_view text)
{
    constexpr std::string_view whitespace = " \t\r\n";
    const auto first = text.find_first_not_of(whitespace);
    if (first == std::string_view::npos)
        return {};
    return text.substr(first, text.find_last_not_of(whitespace) - first + 1);
}

template<class T>
Result<T> number(std::string_view text, const std::string& context)
{
    text = trim(text);
    if (text.empty())
        return parseError("Missing number for " + context + ".");
    T value{};
    const auto result =
        std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size())
        return parseError("Invalid number for " + context + ".");
    return value;
}

std::vector<std::string_view> split(std::string_view text, char delimiter)
{
    std::vector<std::string_view> values;
    std::size_t start = 0;
    do
    {
        const auto end = text.find(delimiter, start);
        values.push_back(trim(text.substr(
            start, end == std::string_view::npos ? end : end - start)));
        if (end == std::string_view::npos)
            break;
        start = end + 1;
    } while (start <= text.size());
    return values;
}

class Tokens
{
public:
    explicit Tokens(std::string_view input) : remaining_(input)
    {
    }
    std::string_view next()
    {
        remaining_ = trim(remaining_);
        const auto end = remaining_.find_first_of(" \t\r\n");
        const auto token = remaining_.substr(0, end);
        remaining_ = end == std::string_view::npos ? std::string_view{}
                                                   : remaining_.substr(end);
        return token;
    }
    bool expect(std::string_view token)
    {
        return next() == token;
    }
    template<class T> Result<T> numeric(const std::string& context)
    {
        return number<T>(next(), context);
    }

private:
    std::string_view remaining_;
};
} // namespace

std::string encodeProject(const PatternDocument& document)
{
    std::ostringstream output;
    output.imbue(std::locale::classic());
    const auto shape = document.shape();
    const auto settings = document.settings();
    output << "VIBMORY 1\nGRID " << shape.rows << ' ' << shape.columns << ' '
           << document.frameCount() << "\nPLAYBACK " << settings.frameRate
           << ' ' << settings.durationMs << ' ' << settings.sampleRate << ' '
           << (settings.phase == PhasePolicy::Continuous ? "continuous"
                                                         : "legacy")
           << "\nROUTING " << shape.cellCount() << '\n';
    output << std::setprecision(std::numeric_limits<double>::max_digits10);
    for (std::size_t i = 0; i < shape.cellCount(); ++i)
        output << document.routing().cells[i] << ' '
               << document.routing().gains[i] << '\n';
    output << "FRAMES\n";
    for (std::size_t index = 0; index < document.frameCount(); ++index)
        for (const auto cell : document.frame(index))
            output << cell.frequencyHz << ' ' << cell.amplitude << '\n';
    output << "END\n";
    return output.str();
}

Result<PatternDocument> decodeProject(std::string_view text)
{
    if (text.size() > maxProjectBytes)
        return parseError("Project exceeds the 16 MiB input limit.");
    Tokens input(text);
    if (!input.expect("VIBMORY") || !input.expect("1"))
        return parseError(
            "Unsupported project signature or version (expected VIBMORY 1).");
    if (!input.expect("GRID"))
        return parseError("Missing GRID section.");
    auto rows = input.numeric<std::uint32_t>("rows");
    auto columns = input.numeric<std::uint32_t>("columns");
    auto frameCount = input.numeric<std::size_t>("frame count");
    if (!rows || !columns || !frameCount)
        return parseError("Invalid GRID dimensions or frame count.");
    auto created = PatternDocument::create({rows.value(), columns.value()});
    if (!created)
        return created.error();
    auto document = std::move(created.value());
    if (frameCount.value() == 0 || frameCount.value() > maxFrames ||
        frameCount.value() * document.shape().cellCount() > maxDocumentCells)
        return parseError("Frame count exceeds the document budget.");
    if (!input.expect("PLAYBACK"))
        return parseError("Missing PLAYBACK section.");
    auto rate = input.numeric<std::uint32_t>("frame rate");
    auto duration = input.numeric<std::uint32_t>("duration ms");
    auto sampleRate = input.numeric<std::uint32_t>("sample rate");
    const auto phase = input.next();
    if (!rate || !duration || !sampleRate ||
        (phase != "continuous" && phase != "legacy"))
        return parseError("Invalid PLAYBACK settings.");
    if (auto result = document.setSettings(
            {rate.value(), duration.value(), sampleRate.value(),
             phase == "continuous" ? PhasePolicy::Continuous
                                   : PhasePolicy::LegacyAbsolute});
        !result)
        return result.error();
    if (!input.expect("ROUTING"))
        return parseError("Missing ROUTING section.");
    auto channels = input.numeric<std::size_t>("channel count");
    if (!channels || channels.value() != document.shape().cellCount())
        return parseError("Channel count does not match the grid.");
    auto routing = ChannelRouting::identity(channels.value());
    for (std::size_t channel = 0; channel < channels.value(); ++channel)
    {
        auto cell = input.numeric<std::size_t>("channel cell index");
        auto gain = input.numeric<double>("channel gain");
        if (!cell || !gain)
            return parseError("Invalid routing value at channel " +
                              std::to_string(channel + 1) + ".");
        routing.cells[channel] = cell.value();
        routing.gains[channel] = gain.value();
    }
    if (auto result = document.setRouting(std::move(routing)); !result)
        return result.error();
    if (!input.expect("FRAMES"))
        return parseError("Missing FRAMES section.");
    for (std::size_t frame = 0; frame < frameCount.value(); ++frame)
    {
        if (frame != 0)
            if (auto result = document.addFrame(); !result)
                return result.error();
        for (std::uint32_t row = 0; row < rows.value(); ++row)
            for (std::uint32_t column = 0; column < columns.value(); ++column)
            {
                auto frequency = input.numeric<std::uint32_t>("frequency");
                auto amplitude = input.numeric<std::uint32_t>("amplitude");
                const auto context = "Frame " + std::to_string(frame + 1) +
                                     ", row " + std::to_string(row + 1) +
                                     ", column " + std::to_string(column + 1) +
                                     ": ";
                if (!frequency || !amplitude)
                    return parseError(context +
                                      "invalid or missing stimulus number.");
                if (auto result = document.setCell(
                        frame, {row, column},
                        {frequency.value(), amplitude.value()});
                    !result)
                    return parseError(context + result.error().message);
            }
    }
    if (!input.expect("END") || !input.next().empty())
        return parseError(
            "Missing END marker or unexpected trailing project content.");
    return document;
}

Result<PatternDocument> decodeLegacyCsv(std::string_view text,
                                        LegacyOrientation orientation)
{
    if (text.size() > maxProjectBytes)
        return parseError("CSV exceeds the 16 MiB input limit.");
    auto lines = split(text, '\n');
    while (!lines.empty() && lines.back().empty())
        lines.pop_back();
    if (lines.empty())
        return parseError("CSV is empty.");
    const auto header = split(lines.front(), ',');
    if (header.size() != 3)
        return parseError(
            "CSV header must contain frame count, dimension, dimension.");
    auto count = number<std::size_t>(header[0], "frame count");
    auto first = number<std::uint32_t>(header[1], "first dimension");
    auto second = number<std::uint32_t>(header[2], "second dimension");
    if (!count || !first || !second)
        return parseError("Invalid CSV header number.");
    const GridShape shape = orientation == LegacyOrientation::RowsThenColumns
                                ? GridShape{first.value(), second.value()}
                                : GridShape{second.value(), first.value()};
    auto created = PatternDocument::create(shape);
    if (!created)
        return created.error();
    if (count.value() == 0 || count.value() > maxFrames ||
        count.value() * shape.cellCount() > maxDocumentCells ||
        lines.size() != 1 + count.value() * shape.rows)
        return parseError(
            "CSV frame count, row count, or document size is invalid.");
    auto document = std::move(created.value());
    auto settings = document.settings();
    settings.phase = PhasePolicy::LegacyAbsolute;
    if (auto result = document.setSettings(settings); !result)
        return result.error();
    auto routing = ChannelRouting::identity(shape.cellCount());
    // Preserve the original audio loop: column first, then row.
    for (std::uint32_t column = 0; column < shape.columns; ++column)
        for (std::uint32_t row = 0; row < shape.rows; ++row)
            routing.cells[static_cast<std::size_t>(column) * shape.rows + row] =
                static_cast<std::size_t>(row) * shape.columns + column;
    if (auto result = document.setRouting(std::move(routing)); !result)
        return result.error();
    for (std::size_t frame = 0; frame < count.value(); ++frame)
    {
        if (frame != 0)
            if (auto result = document.addFrame(); !result)
                return result.error();
        for (std::uint32_t row = 0; row < shape.rows; ++row)
        {
            const auto lineIndex = 1 + frame * shape.rows + row;
            auto fields = split(lines[lineIndex], ',');
            if (!fields.empty() && fields.back().empty())
                fields.pop_back();
            if (fields.size() != shape.columns * 2)
                return parseError("CSV line " + std::to_string(lineIndex + 1) +
                                  ": wrong field count.");
            for (std::uint32_t column = 0; column < shape.columns; ++column)
            {
                const auto context = "CSV line " +
                                     std::to_string(lineIndex + 1) + ", cell " +
                                     std::to_string(column + 1);
                auto frequency = number<std::uint32_t>(fields[column * 2],
                                                       context + " frequency");
                auto amplitude = number<std::uint32_t>(fields[column * 2 + 1],
                                                       context + " amplitude");
                if (!frequency || !amplitude)
                    return parseError(context + ": invalid stimulus number.");
                if (auto result = document.setCell(
                        frame, {row, column},
                        {frequency.value(), amplitude.value()});
                    !result)
                    return parseError(context + ": " + result.error().message);
            }
        }
    }
    return document;
}
} // namespace vibmory
