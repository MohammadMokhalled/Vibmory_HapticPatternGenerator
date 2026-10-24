#pragma once

#include "domain/result.h"

#include <filesystem>
#include <string_view>

namespace vibmory
{
[[nodiscard]] Result<std::string> readTextFile(
    const std::filesystem::path& path, std::size_t maximumBytes);
[[nodiscard]] Result<void> writeTextFileAtomically(
    const std::filesystem::path& path, std::string_view text);
} // namespace vibmory
