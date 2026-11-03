#pragma once

#include "domain/result.h"

#include <filesystem>
#include <functional>
#include <string_view>

namespace vibmory
{
[[nodiscard]] Result<std::string> readTextFile(
    const std::filesystem::path& path, std::size_t maximumBytes);
[[nodiscard]] Result<void> writeTextFileAtomically(
    const std::filesystem::path& path, std::string_view text,
    const std::function<bool()>& continueWriting = {});
} // namespace vibmory
