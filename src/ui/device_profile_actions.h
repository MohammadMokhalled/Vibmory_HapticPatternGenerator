#pragma once

#include "application/editor_session.h"
#include "application/playback_service.h"
#include "domain/device_profile.h"

#include <QObject>

#include <functional>
#include <optional>

class QAction;
class QMenu;
class QWidget;

namespace vibmory
{
class DeviceProfileActions final : public QObject
{
    Q_OBJECT
public:
    DeviceProfileActions(EditorSession& session, PlaybackBackend& backend,
                         QMenu& menu, std::function<void()> changed,
                         QWidget* parent);
    void setEnabled(bool enabled);
    [[nodiscard]] static std::optional<bool> reviewProfile(
        const DeviceProfile& profile,
        const std::vector<AudioDeviceDescription>& devices, QWidget* parent);

private:
    void saveProfile();
    void loadProfile();

    EditorSession& session_;
    PlaybackBackend& backend_;
    std::function<void()> changed_;
    QWidget* window_;
    QAction* save_;
    QAction* load_;
};
} // namespace vibmory
