#include "ui/editor_window.h"

#include "platform/project_store.h"
#include "platform/wave_export_job.h"
#include "ui/cell_inspector.h"
#include "ui/device_profile_actions.h"
#include "ui/file_dialogs.h"
#include "ui/frame_list_model.h"
#include "ui/grid_widget.h"
#include "ui/recovery_controller.h"
#include "ui/ui_preferences.h"

#include <QAction>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QListView>
#include <QLoggingCategory>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPolygonF>
#include <QProgressDialog>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyle>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace vibmory
{
namespace
{
Q_LOGGING_CATEGORY(uiLog, "vibmory.ui")

QIcon mediaIcon(bool stopping, const QPalette& palette)
{
    QIcon icon;
    for (auto mode : {QIcon::Normal, QIcon::Disabled})
    {
        QPixmap pixmap(32, 32);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(palette.color(
            mode == QIcon::Disabled ? QPalette::Disabled : QPalette::Active,
            QPalette::ButtonText));
        if (stopping)
            painter.drawRect(QRectF(8, 8, 16, 16));
        else
            painter.drawPolygon(QPolygonF{{10, 6}, {26, 16}, {10, 26}});
        painter.end();
        icon.addPixmap(pixmap, mode);
    }
    return icon;
}
} // namespace

EditorWindow::EditorWindow(DocumentStore& store, PlaybackBackend& backend,
                           const MonotonicClock& clock, WaveExportJob& exporter,
                           PatternDocument document, QWidget* parent)
    : QMainWindow(parent), session_(std::move(document)),
      documents_(session_, store), backend_(backend), playback_(backend, clock),
      exporter_(exporter)
{
    setObjectName("editorWindow");
    resize(1100, 720);
    buildMenus();
    buildEditor();
    refreshTimer_.setInterval(25);
    connect(&refreshTimer_, &QTimer::timeout, this, [this] {
        playback_.tick();
        refresh();
    });
    connect(&exporter_, &WaveExportJob::progressChanged, this,
            [this](int value) {
                if (exportDialog_)
                    exportDialog_->setValue(value);
            });
    connect(&exporter_, &WaveExportJob::finished, this,
            [this](const Result<void>& result) {
                if (exportDialog_)
                {
                    exportDialog_->close();
                    exportDialog_->deleteLater();
                    exportDialog_ = nullptr;
                }
                if (!result && result.error().code != ErrorCode::Cancelled)
                    showError(result.error());
                else
                    statusBar()->showMessage(result ? tr("WAV export complete.")
                                                    : tr("Export cancelled."),
                                             5000);
                refresh();
            });
    refreshDevices();
    refresh();
    refreshIcons();
}

EditorWindow::~EditorWindow()
{
    recovery_.reset();
    refreshTimer_.stop();
    playback_.stop();
    exporter_.cancelAndWait();
}

void EditorWindow::enableRecovery(const QString& directory, bool offerRestore)
{
    recovery_ = std::make_unique<RecoveryController>(session_, directory);
    connect(recovery_.get(), &RecoveryController::warningOccurred, this,
            [this](const QString& message) {
                statusBar()->showMessage(tr("Recovery: %1").arg(message),
                                         10000);
            });
    if (offerRestore)
        (void)recovery_->offerRestore(this);
    refresh();
}

void EditorWindow::enablePreferences(UiPreferences& preferences)
{
    if (findChild<QMenu*>("viewMenu"))
        return;
    auto* view = new QMenu(tr("&View"), this);
    view->setObjectName("viewMenu");
    menuBar()->insertMenu(menuBar()->actions().last(), view);
    preferences.populateMenu(*view);
    connect(&preferences, &UiPreferences::themeChanged, this,
            &EditorWindow::refreshIcons);
    connect(&preferences, &UiPreferences::notice, this,
            [this](const QString& message) {
                statusBar()->showMessage(message, 8000);
            });
}

void EditorWindow::refreshIcons()
{
    frameModel_->setPalette(palette());
    new_->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    open_->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    save_->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    import_->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
    export_->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    undo_->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    redo_->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    const std::pair<const char*, QStyle::StandardPixmap> icons[] = {
        {"addFrameButton", QStyle::SP_FileIcon},
        {"duplicateFrameButton", QStyle::SP_FileLinkIcon},
        {"deleteFrameButton", QStyle::SP_TrashIcon},
        {"moveEarlierButton", QStyle::SP_ArrowUp},
        {"moveLaterButton", QStyle::SP_ArrowDown},
        {"refreshDevicesButton", QStyle::SP_BrowserReload}};
    for (const auto& [name, icon] : icons)
        findChild<QPushButton*>(name)->setIcon(style()->standardIcon(icon));
    play_->setIcon(
        mediaIcon(playback_.state() == PlaybackState::Playing, palette()));
    setWindowIcon(style()->standardIcon(QStyle::SP_MediaVolume));
}

void EditorWindow::buildMenus()
{
    auto* file = menuBar()->addMenu(tr("&File"));
    new_ = file->addAction(tr("&New project…"), QKeySequence::New, this,
                           &EditorWindow::newProject);
    open_ = file->addAction(tr("&Open project…"), QKeySequence::Open, this,
                            [this] { openProject(); });
    import_ = file->addAction(tr("Import legacy &CSV…"), this,
                              [this] { openProject(true); });
    file->addSeparator();
    save_ = file->addAction(tr("&Save"), QKeySequence::Save, this,
                            [this] { (void)saveProject(); });
    file->addAction(tr("Save &as…"), QKeySequence::SaveAs, this,
                    [this] { (void)saveProject(true); });
    export_ = file->addAction(tr("Export &WAV…"), QKeySequence("Ctrl+E"), this,
                              &EditorWindow::exportWave);
    file->addSeparator();
    file->addAction(tr("E&xit"), QKeySequence::Quit, this, &QWidget::close);
    auto* edit = menuBar()->addMenu(tr("&Edit"));
    undo_ = edit->addAction(tr("&Undo"), QKeySequence::Undo, this, [this] {
        session_.undo();
        refresh();
    });
    redo_ = edit->addAction(tr("&Redo"), QKeySequence::Redo, this, [this] {
        session_.redo();
        refresh();
    });
    routing_ = edit->addAction(tr("Channel &routing and gains…"), this,
                               &EditorWindow::configureRouting);
    edit->addSeparator();
    profileActions_ = new DeviceProfileActions(
        session_, backend_, *edit,
        [this] {
            refreshDevices();
            refresh();
        },
        this);
    auto* help = menuBar()->addMenu(tr("&Help"));
    help->addAction(tr("&About Vibmory"), this, [this] {
        QMessageBox::about(
            this, tr("About Vibmory"),
            tr("Vibmory %1\nHaptic pattern editor\n\nQt 6 desktop edition. "
               "See README and THIRD_PARTY_NOTICES in the application folder "
               "for project and dependency information.")
                .arg(QCoreApplication::applicationVersion()));
    });
}

void EditorWindow::buildEditor()
{
    auto* central = new QWidget(this);
    auto* outer = new QVBoxLayout(central);
    editControls_ = new QWidget(central);
    auto* layout = new QHBoxLayout(editControls_);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* sequence = new QVBoxLayout;
    auto* frameLabel = new QLabel(tr("Frames"));
    frames_ = new QListView;
    frameModel_ = new FrameListModel(
        [this](std::uint64_t id, int target, std::uint64_t revision) {
            if (revision != session_.revision() || exporter_.busy() ||
                (playback_.state() != PlaybackState::Idle &&
                 playback_.state() != PlaybackState::Error))
                return false;
            const auto& document = session_.document();
            for (std::size_t index = 0; index < document.frameCount(); ++index)
                if (document.frameId(index) == id)
                    return static_cast<bool>(session_.moveFrame(
                        index, static_cast<std::size_t>(target)));
            return false;
        },
        frames_);
    frames_->setModel(frameModel_);
    frames_->setObjectName("frameList");
    frames_->setAccessibleName(tr("Frame sequence"));
    frames_->setDragDropMode(QAbstractItemView::InternalMove);
    frames_->setDefaultDropAction(Qt::MoveAction);
    frames_->setDragDropOverwriteMode(false);
    frames_->setSelectionMode(QAbstractItemView::SingleSelection);
    frames_->setIconSize({80, 48});
    frames_->setSpacing(3);
    frames_->setMaximumWidth(200);
    sequence->addWidget(frameLabel);
    sequence->addWidget(frames_);
    auto addFrameButton = [this, sequence](const QString& text, auto command) {
        auto* button = new QPushButton(text);
        sequence->addWidget(button);
        connect(button, &QPushButton::clicked, this, command);
        return button;
    };
    addFrameButton(tr("Add frame"), [this] {
        apply(session_.addFrame());
    })->setObjectName("addFrameButton");
    addFrameButton(tr("Duplicate"), [this] {
        apply(session_.duplicateFrame());
    })->setObjectName("duplicateFrameButton");
    deleteFrame_ =
        addFrameButton(tr("Delete"), [this] { apply(session_.removeFrame()); });
    deleteFrame_->setObjectName("deleteFrameButton");
    addFrameButton(tr("Move earlier"), [this] {
        const auto index = session_.selectedFrame();
        if (index > 0)
            apply(session_.moveFrame(index, index - 1));
    })->setObjectName("moveEarlierButton");
    addFrameButton(tr("Move later"), [this] {
        const auto index = session_.selectedFrame();
        if (index + 1 < session_.document().frameCount())
            apply(session_.moveFrame(index, index + 1));
    })->setObjectName("moveLaterButton");
    layout->addLayout(sequence);
    grid_ = new GridWidget;
    layout->addWidget(grid_, 1);
    auto* side = new QVBoxLayout;
    inspector_ = new CellInspector;
    connect(inspector_, &CellInspector::stimulusEdited, this,
            [this] { apply(session_.editCell(inspector_->stimulus())); });
    connect(inspector_, &CellInspector::editGroupStarted, this,
            [this] { session_.beginEditGroup(); });
    connect(inspector_, &CellInspector::editGroupFinished, this,
            [this] { session_.endEditGroup(); });
    side->addWidget(inspector_);
    auto* settings = new QGroupBox(tr("Playback settings"));
    auto* form = new QFormLayout(settings);
    frameRate_ = new QSpinBox;
    frameRate_->setRange(1, 240);
    frameRate_->setObjectName("frameRateSpinBox");
    duration_ = new QDoubleSpinBox;
    duration_->setDecimals(3);
    duration_->setRange(0.001, 600);
    duration_->setSuffix(tr(" s"));
    sampleRate_ = new QComboBox;
    for (int rate : {8000, 16000, 22050, 44100, 48000, 96000, 192000})
        sampleRate_->addItem(QString::number(rate), rate);
    phase_ = new QComboBox;
    phase_->addItem(tr("Continuous phase"),
                    static_cast<int>(PhasePolicy::Continuous));
    phase_->addItem(tr("Legacy absolute phase"),
                    static_cast<int>(PhasePolicy::LegacyAbsolute));
    form->addRow(tr("Frame rate (frames/s)"), frameRate_);
    form->addRow(tr("Duration"), duration_);
    form->addRow(tr("Sample rate (Hz)"), sampleRate_);
    form->addRow(tr("Oscillator"), phase_);
    side->addWidget(settings);
    devices_ = new QComboBox;
    devices_->setObjectName("outputDeviceComboBox");
    devices_->setAccessibleName(tr("Audio output device"));
    side->addWidget(new QLabel(tr("Output device")));
    side->addWidget(devices_);
    auto* reload = new QPushButton(tr("Refresh devices"));
    reload->setObjectName("refreshDevicesButton");
    side->addWidget(reload);
    side->addStretch();
    layout->addLayout(side);
    outer->addWidget(editControls_, 1);
    play_ = new QPushButton(tr("Play"));
    play_->setObjectName("playButton");
    outer->addWidget(play_);
    setCentralWidget(central);
    connect(play_, &QPushButton::clicked, this, &EditorWindow::togglePlayback);
    connect(reload, &QPushButton::clicked, this, &EditorWindow::refreshDevices);
    connect(frameModel_, &FrameListModel::documentReordered, this,
            &EditorWindow::refresh);
    connect(frames_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex& current) {
                if (current.isValid() && !frameModel_->moving())
                    apply(session_.selectFrame(
                        static_cast<std::size_t>(current.row())));
            });
    connect(grid_, &GridWidget::cellSelected, this,
            [this](int row, int column) {
                apply(session_.selectCell(
                    CellPosition{static_cast<std::uint32_t>(row),
                                 static_cast<std::uint32_t>(column)}));
            });
    connect(grid_, &GridWidget::selectionCleared, this,
            [this] { apply(session_.selectCell(std::nullopt)); });
    connect(frameRate_, &QSpinBox::valueChanged, this,
            &EditorWindow::updateSettings);
    connect(duration_, &QDoubleSpinBox::valueChanged, this,
            &EditorWindow::updateSettings);
    connect(sampleRate_, &QComboBox::currentIndexChanged, this,
            &EditorWindow::updateSettings);
    connect(phase_, &QComboBox::currentIndexChanged, this,
            &EditorWindow::updateSettings);
    connect(devices_, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0)
            apply(backend_.selectDeviceId(
                devices_->itemData(index).toString().toStdString()));
    });
}

void EditorWindow::refresh()
{
    if (recovery_)
        recovery_->observe();
    const bool playing = playback_.state() == PlaybackState::Playing;
    const bool busy = playing || exporter_.busy();
    profileActions_->setEnabled(!busy);
    editControls_->setEnabled(!busy);
    deleteFrame_->setEnabled(!busy && session_.document().frameCount() > 1);
    for (auto* action : {new_, open_, import_, export_, routing_})
        action->setEnabled(!busy);
    undo_->setEnabled(!busy && session_.canUndo());
    redo_->setEnabled(!busy && session_.canRedo());
    play_->setEnabled(!exporter_.busy());
    const auto playText = playing ? tr("Stop") : tr("Play");
    if (play_->text() != playText)
    {
        play_->setText(playText);
        play_->setIcon(mediaIcon(playing, palette()));
    }
    const QSignalBlocker frameBlocker(frames_->selectionModel());
    frameModel_->setDocument(session_.document(), session_.revision());
    frames_->setCurrentIndex(
        frameModel_->index(static_cast<int>(session_.selectedFrame())));
    const auto settings = session_.document().settings();
    const QSignalBlocker rateBlocker(frameRate_), durationBlocker(duration_),
        sampleBlocker(sampleRate_), phaseBlocker(phase_);
    frameRate_->setValue(static_cast<int>(settings.frameRate));
    duration_->setValue(settings.durationMs / 1000.0);
    auto sampleIndex =
        sampleRate_->findData(static_cast<int>(settings.sampleRate));
    if (sampleIndex < 0)
    {
        sampleRate_->addItem(QString::number(settings.sampleRate),
                             static_cast<int>(settings.sampleRate));
        sampleIndex = sampleRate_->count() - 1;
    }
    sampleRate_->setCurrentIndex(sampleIndex);
    phase_->setCurrentIndex(phase_->findData(static_cast<int>(settings.phase)));
    std::optional<CellStimulus> selectedValue;
    if (session_.selectedCell())
        selectedValue =
            session_.document()
                .cell(session_.selectedFrame(), *session_.selectedCell())
                .value();
    inspector_->setCell(session_.selectedCell(), selectedValue,
                        std::min(maxFrequencyHz, settings.sampleRate / 2 - 1));
    if (playing && playback_.snapshot())
        grid_->setFrame(playback_.snapshot()->document, playback_.visualFrame(),
                        std::nullopt);
    else
        grid_->setFrame(session_.document(), session_.selectedFrame(),
                        session_.selectedCell());
    setWindowTitle(tr("%1[*] — Vibmory")
                       .arg(session_.path().empty()
                                ? tr("Untitled")
                                : toQString(session_.path().filename())));
    setWindowModified(session_.dirty());
    if (playing)
    {
        if (!refreshTimer_.isActive())
            refreshTimer_.start();
    }
    else
        refreshTimer_.stop();
}

void EditorWindow::apply(Result<void> result)
{
    if (!result)
        showError(result.error());
    refresh();
}
void EditorWindow::showError(const Error& error)
{
    qCWarning(uiLog).noquote() << static_cast<int>(error.code)
                               << QString::fromStdString(error.message);
    QMessageBox::critical(this, tr("Vibmory"),
                          QString::fromStdString(error.message));
}

void EditorWindow::updateSettings()
{
    apply(session_.setSettings(
        {static_cast<std::uint32_t>(frameRate_->value()),
         static_cast<std::uint32_t>(qRound(duration_->value() * 1000)),
         sampleRate_->currentData().toUInt(),
         static_cast<PhasePolicy>(phase_->currentData().toInt())}));
}

bool EditorWindow::saveProject(bool saveAs)
{
    auto path = session_.path();
    if (saveAs || path.empty())
    {
        const auto selected = chooseSavePath(this, tr("Save project"),
                                             tr("Vibmory project (*.vibmory)"),
                                             "vibmory", toQString(path));
        if (selected.isEmpty())
            return false;
        path = toPath(selected);
    }
    const auto result = documents_.save(path);
    if (!result)
    {
        showError(result.error());
        return false;
    }
    const bool current = result.value().savedCurrentRevision;
    statusBar()->showMessage(
        current ? tr("Project saved.")
                : tr("Snapshot saved; newer changes remain unsaved."),
        3000);
    refresh();
    return current;
}

bool EditorWindow::confirmDiscard()
{
    if (!session_.dirty())
        return true;
    const auto choice = QMessageBox::warning(
        this, tr("Unsaved changes"), tr("Save changes before continuing?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);
    return choice == QMessageBox::Discard ||
           (choice == QMessageBox::Save && saveProject());
}

Result<void> EditorWindow::loadProject(const std::filesystem::path& path)
{
    auto result = documents_.read(path);
    if (!result)
        return result.error();
    playback_.stop();
    documents_.replace(std::move(result.value()), path);
    refresh();
    return {};
}

void EditorWindow::openProject(bool legacy)
{
    const auto selected = QFileDialog::getOpenFileName(
        this, legacy ? tr("Import legacy CSV") : tr("Open project"), {},
        legacy ? tr("Legacy CSV (*.csv);;All files (*)")
               : tr("Vibmory project (*.vibmory);;All files (*)"));
    if (selected.isEmpty())
        return;
    Result<PatternDocument> loaded = Error{ErrorCode::Cancelled, "Cancelled"};
    if (legacy)
    {
        bool accepted = false;
        const auto choice = QInputDialog::getItem(
            this, tr("Legacy grid orientation"),
            tr("The old writer and reader disagreed about dimensions. Select "
               "the header convention:"),
            {tr("Frame count, rows, columns (original writer)"),
             tr("Frame count, columns, rows (alternate)")},
            0, false, &accepted);
        if (!accepted)
            return;
        const auto orientation =
            choice == tr("Frame count, rows, columns (original writer)")
                ? LegacyOrientation::RowsThenColumns
                : LegacyOrientation::ColumnsThenRows;
        loaded = importLegacy(toPath(selected), orientation);
    }
    else
        loaded = documents_.read(toPath(selected));
    if (!loaded)
    {
        showError(loaded.error());
        return;
    }
    if (!confirmDiscard())
        return;
    documents_.replace(std::move(loaded.value()),
                       legacy ? std::filesystem::path{} : toPath(selected));
    refresh();
}

void EditorWindow::newProject()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("New project"));
    auto* layout = new QFormLayout(&dialog);
    auto* rows = new QSpinBox;
    rows->setRange(1, static_cast<int>(maxCells));
    rows->setValue(2);
    auto* columns = new QSpinBox;
    columns->setRange(1, static_cast<int>(maxCells));
    columns->setValue(2);
    layout->addRow(tr("Rows"), rows);
    layout->addRow(tr("Columns"), columns);
    auto* buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        auto result = PatternDocument::create(
            {static_cast<std::uint32_t>(rows->value()),
             static_cast<std::uint32_t>(columns->value())});
        if (!result)
            showError(result.error());
        else
            dialog.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted || !confirmDiscard())
        return;
    auto document =
        PatternDocument::create({static_cast<std::uint32_t>(rows->value()),
                                 static_cast<std::uint32_t>(columns->value())});
    documents_.replace(std::move(document.value()));
    refresh();
}

void EditorWindow::togglePlayback()
{
    if (playback_.state() == PlaybackState::Playing)
        playback_.stop();
    else
    {
        const auto result = playback_.start(session_.snapshot());
        if (!result)
            showError(result.error());
    }
    refresh();
}

void EditorWindow::reportBackendFailure(const QString& message)
{
    if (playback_.state() != PlaybackState::Playing)
        return;
    playback_.fail({ErrorCode::Device, message.toStdString()});
    refresh();
    showError(*playback_.lastError());
}

void EditorWindow::playbackFinished()
{
    playback_.stop();
    refresh();
}

void EditorWindow::refreshDevices()
{
    const auto selected = QString::fromStdString(backend_.selectedDeviceId());
    const QSignalBlocker blocker(devices_);
    devices_->clear();
    for (const auto& device : backend_.devices())
        devices_->addItem(QString::fromStdString(device.label),
                          QString::fromStdString(device.id));
    const int index = devices_->findData(selected);
    devices_->setCurrentIndex(index);
    if (devices_->count() == 0)
        devices_->setPlaceholderText(
            tr("No audio output (WAV export available)"));
    else if (index < 0)
        devices_->setPlaceholderText(tr("Select an output device"));
}

void EditorWindow::exportWave()
{
    const auto selected =
        chooseSavePath(this, tr("Export WAV"), tr("WAV audio (*.wav)"), "wav");
    if (selected.isEmpty())
        return;
    exportDialog_ = new QProgressDialog(tr("Rendering audio..."), tr("Cancel"),
                                        0, 100, this);
    exportDialog_->setWindowModality(Qt::WindowModal);
    exportDialog_->setAutoClose(false);
    exportDialog_->setAutoReset(false);
    connect(exportDialog_, &QProgressDialog::canceled, &exporter_,
            &WaveExportJob::cancel);
    const auto result = exporter_.start(toPath(selected), session_.snapshot());
    if (!result)
    {
        exportDialog_->deleteLater();
        exportDialog_ = nullptr;
        showError(result.error());
    }
    refresh();
}

void EditorWindow::configureRouting()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Physical channel routing and gains"));
    dialog.resize(560, 500);
    auto* layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(tr("Each output channel maps to exactly one "
                                    "logical cell. Gains range from 0 to 1.")));
    const auto& document = session_.document();
    auto* table =
        new QTableWidget(static_cast<int>(document.shape().cellCount()), 3);
    table->setHorizontalHeaderLabels(
        {tr("Channel"), tr("Logical cell"), tr("Gain")});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    for (std::size_t i = 0; i < document.shape().cellCount(); ++i)
    {
        auto* channel = new QTableWidgetItem(QString::number(i + 1));
        channel->setFlags(Qt::ItemIsEnabled);
        table->setItem(static_cast<int>(i), 0, channel);
        auto* cell = new QComboBox;
        for (std::size_t j = 0; j < document.shape().cellCount(); ++j)
            cell->addItem(tr("R%1 C%2")
                              .arg(j / document.shape().columns + 1)
                              .arg(j % document.shape().columns + 1),
                          static_cast<int>(j));
        cell->setCurrentIndex(static_cast<int>(document.routing().cells[i]));
        auto* gain = new QDoubleSpinBox;
        gain->setDecimals(4);
        gain->setRange(0, 1);
        gain->setSingleStep(0.05);
        gain->setValue(document.routing().gains[i]);
        table->setCellWidget(static_cast<int>(i), 1, cell);
        table->setCellWidget(static_cast<int>(i), 2, gain);
    }
    layout->addWidget(table);
    auto* buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        auto routing = document.routing();
        for (std::size_t i = 0; i < routing.cells.size(); ++i)
        {
            routing.cells[i] = static_cast<std::size_t>(
                qobject_cast<QComboBox*>(
                    table->cellWidget(static_cast<int>(i), 1))
                    ->currentData()
                    .toUInt());
            routing.gains[i] = qobject_cast<QDoubleSpinBox*>(
                                   table->cellWidget(static_cast<int>(i), 2))
                                   ->value();
        }
        auto result = session_.setRouting(std::move(routing));
        if (!result)
            showError(result.error());
        else
            dialog.accept();
    });
    dialog.exec();
    refresh();
}

void EditorWindow::closeEvent(QCloseEvent* event)
{
    if (!confirmDiscard())
    {
        event->ignore();
        return;
    }
    playback_.stop();
    refreshTimer_.stop();
    exporter_.cancelAndWait();
    if (recovery_)
        recovery_->clear();
    event->accept();
}
} // namespace vibmory
