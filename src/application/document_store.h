#pragma once

#include "domain/pattern.h"

#include <filesystem>

namespace vibmory
{
class DocumentStore
{
        public:
    virtual ~DocumentStore() = default;
    [[nodiscard]] virtual Result<PatternDocument> load(
        const std::filesystem::path &path) const = 0;
    [[nodiscard]] virtual Result<void> save(
        const std::filesystem::path &path,
        const PatternDocument &document) const = 0;
};
} // namespace vibmory
