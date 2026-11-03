#pragma once

#include "domain/pattern.h"

#include <string_view>

namespace vibmory
{
inline constexpr std::size_t maxProjectBytes = 16 * 1024 * 1024;
enum class LegacyOrientation
{
    RowsThenColumns,
    ColumnsThenRows
};

[[nodiscard]] std::string encodeProject(const PatternDocument& document);
[[nodiscard]] Result<PatternDocument> decodeProject(std::string_view text);
// Explicit orientation is required: the original writer and reader disagreed.
[[nodiscard]] Result<PatternDocument> decodeLegacyCsv(
    std::string_view text, LegacyOrientation orientation);
} // namespace vibmory
