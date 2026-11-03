#include "domain/device_profile.h"

#include <algorithm>

namespace vibmory
{
Result<void> DeviceProfile::validate() const
{
    if (name.empty() || name.size() > 128 ||
        std::all_of(name.begin(), name.end(),
                    [](unsigned char c) { return c == ' '; }) ||
        std::any_of(name.begin(), name.end(),
                    [](unsigned char c) { return c < 32 || c == 127; }))
        return invalid("Profile name must contain 1-128 bytes without control "
                       "characters.");
    if (outputDeviceId.size() > 1024 ||
        outputDeviceId.find('\0') != std::string::npos)
        return invalid(
            "Profile output device identity is invalid or too long.");
    if (auto result = shape.validate(); !result)
        return result;
    return routing.validate(shape.cellCount());
}
} // namespace vibmory
