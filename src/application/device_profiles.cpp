#include "application/device_profiles.h"

#include <algorithm>

namespace vibmory
{
Result<void> applyDeviceProfile(EditorSession& session,
                                PlaybackBackend& backend,
                                const DeviceProfile& profile,
                                bool selectOutputDevice)
{
    if (auto result = profile.validate(); !result)
        return result;
    if (profile.shape != session.document().shape())
        return invalid(
            "Profile grid dimensions do not match the current project.");
    if (selectOutputDevice)
    {
        const auto devices = backend.devices();
        if (profile.outputDeviceId.empty() ||
            std::none_of(devices.begin(), devices.end(),
                         [&](const auto& device) {
                             return device.id == profile.outputDeviceId;
                         }))
            return Error{ErrorCode::Device,
                         "The profile's output device is unavailable."};
        if (auto result = backend.selectDeviceId(profile.outputDeviceId);
            !result)
            return result;
    }
    return session.setRouting(profile.routing);
}
} // namespace vibmory
