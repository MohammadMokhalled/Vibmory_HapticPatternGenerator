#pragma once

#include "domain/device_profile.h"

#include <filesystem>

namespace vibmory
{
inline constexpr std::size_t maxDeviceProfileBytes = 65536;

[[nodiscard]] Result<DeviceProfile> loadDeviceProfile(
    const std::filesystem::path& path);
[[nodiscard]] Result<void> saveDeviceProfile(const std::filesystem::path& path,
                                             const DeviceProfile& profile);
} // namespace vibmory
