#pragma once

#include "domain/pattern.h"

#include <string>

namespace vibmory
{
struct DeviceProfile
{
    std::string name;
    GridShape shape;
    ChannelRouting routing;
    // An optional, opaque backend identity. It may not exist on another
    // machine.
    std::string outputDeviceId;
    [[nodiscard]] Result<void> validate() const;
    bool operator==(const DeviceProfile&) const = default;
};
} // namespace vibmory
