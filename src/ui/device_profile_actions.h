#pragma once

#include "application/editor_session.h"
#include "application/playback_service.h"

#include <QObject>

#include <functional>

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
