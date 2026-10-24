#pragma once

#include "application/editor_session.h"
#include "application/playback_service.h"
#include "domain/device_profile.h"

namespace vibmory
{
// Applies a validated profile to the current grid. Device selection is opt-in;
// failure to select it must leave the document unchanged.
[[nodiscard]] Result<void> applyDeviceProfile(EditorSession& session,
                                              PlaybackBackend& backend,
                                              const DeviceProfile& profile,
                                              bool selectOutputDevice);
} // namespace vibmory
