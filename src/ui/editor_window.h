#pragma once

#include "application/document_store.h"
#include "application/document_workflow.h"
#include "application/editor_session.h"
#include "application/playback_service.h"

#include <QMainWindow>
#include <QTimer>

#include <memory>

class QAction;
class QComboBox;
class QDoubleSpinBox;
class QListView;
class QProgressDialog;
class QPushButton;
class QSpinBox;

namespace vibmory
{
class GridWidget;
class CellInspector;
class FrameListModel;
class DeviceProfileActions;
class RecoveryController;
class WaveExportJob;
class UiPreferences;

class EditorWindow final : public QMainWindow
{
    Q_OBJECT
public:
    EditorWindow(DocumentStore& store, PlaybackBackend& backend,
                 const MonotonicClock& clock, WaveExportJob& exporter,
                 PatternDocument document, QWidget* parent = nullptr);
    ~EditorWindow() override;
    [[nodiscard]] const EditorSession& session() const
    {
        return session_;
    }
    [[nodiscard]] Result<void> loadProject(const std::filesystem::path& path);
    void reportBackendFailure(const QString& message);
    void playbackFinished();
    void enableRecovery(const QString& directory, bool offerRestore);
    void enablePreferences(UiPreferences& preferences);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void buildMenus();
    void buildEditor();
    void refresh();
    void apply(Result<void> result);
    void showError(const Error& error);
    void newProject();
    void openProject(bool legacy = false);
    [[nodiscard]] bool saveProject(bool saveAs = false);
    [[nodiscard]] bool confirmDiscard();
    void updateSettings();
    void togglePlayback();
    void exportWave();
    void configureRouting();
    void refreshDevices();
    void refreshIcons();

    EditorSession session_;
    DocumentWorkflow documents_;
    PlaybackBackend& backend_;
    PlaybackService playback_;
    WaveExportJob& exporter_;
    std::unique_ptr<RecoveryController> recovery_;
    QTimer refreshTimer_;
    QProgressDialog* exportDialog_ = nullptr;
    GridWidget* grid_ = nullptr;
    QListView* frames_ = nullptr;
    FrameListModel* frameModel_ = nullptr;
    DeviceProfileActions* profileActions_ = nullptr;
    QWidget* editControls_ = nullptr;
    CellInspector* inspector_ = nullptr;
    QSpinBox* frameRate_ = nullptr;
    QDoubleSpinBox* duration_ = nullptr;
    QComboBox* sampleRate_ = nullptr;
    QComboBox* phase_ = nullptr;
    QComboBox* devices_ = nullptr;
    QPushButton* play_ = nullptr;
    QPushButton* deleteFrame_ = nullptr;
    QAction* undo_ = nullptr;
    QAction* redo_ = nullptr;
    QAction* new_ = nullptr;
    QAction* open_ = nullptr;
    QAction* import_ = nullptr;
    QAction* export_ = nullptr;
    QAction* routing_ = nullptr;
    QAction* save_ = nullptr;
};
} // namespace vibmory
