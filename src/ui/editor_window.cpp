#include "ui/editor_window.h"

#include "platform/project_store.h"
#include "ui/grid_widget.h"

#include <QAction>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QtConcurrentRun>

#include <algorithm>

namespace vibmory
{
EditorWindow::EditorWindow(DocumentStore &store, PlaybackBackend &backend,
                           const MonotonicClock &clock,
                           PatternDocument document, QWidget *parent)
    : QMainWindow(parent), session_(std::move(document)), store_(store),
      backend_(backend), playback_(backend, clock)
{
    setObjectName("editorWindow");
    resize(1100, 720);
    buildMenus();
    buildEditor();
    refreshTimer_.setInterval(25);
    connect(&refreshTimer_, &QTimer::timeout, this, [this] {
        playback_.tick();
        if(exportDialog_ && exportState_)
            exportDialog_->setValue(static_cast<int>(
                exportState_->done.load() * 100 / exportState_->total.load()));
        refresh();
    });
    connect(&exportWatcher_, &QFutureWatcher<Result<void>>::finished, this,
            [this] {
                const auto result = exportWatcher_.result();
                if(exportDialog_)
                {
                    exportDialog_->close();
                    exportDialog_->deleteLater();
                    exportDialog_ = nullptr;
                }
                exportState_.reset();
                if(!result && result.error().code != ErrorCode::Cancelled)
                    showError(result.error());
                else
                    statusBar()->showMessage(result ? tr("WAV export complete.")
                                                    : tr("Export cancelled."),
                                             5000);
                refresh();
            });
    refreshDevices();
    refresh();
}

EditorWindow::~EditorWindow()
{
    refreshTimer_.stop();
    playback_.stop();
    if(exportState_)
        exportState_->cancel = true;
    exportWatcher_.waitForFinished();
}

void EditorWindow::buildMenus()
{
    auto *file = menuBar()->addMenu(tr("&File"));
    new_ = file->addAction(tr("&New project…"), QKeySequence::New, this,
                           &EditorWindow::newProject);
    open_ = file->addAction(tr("&Open project…"), QKeySequence::Open, this,
                            [this] { openProject(); });
    import_ = file->addAction(tr("Import legacy &CSV…"), this,
                              [this] { openProject(true); });
    file->addSeparator();
    file->addAction(tr("&Save"), QKeySequence::Save, this,
                    [this] { (void)saveProject(); });
    file->addAction(tr("Save &as…"), QKeySequence::SaveAs, this,
                    [this] { (void)saveProject(true); });
    export_ = file->addAction(tr("Export &WAV…"), QKeySequence(tr("Ctrl+E")),
                              this, &EditorWindow::exportWave);
    file->addSeparator();
    file->addAction(tr("E&xit"), QKeySequence::Quit, this, &QWidget::close);
    auto *edit = menuBar()->addMenu(tr("&Edit"));
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
    auto *help = menuBar()->addMenu(tr("&Help"));
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
    auto *central = new QWidget(this);
    auto *outer = new QVBoxLayout(central);
    editControls_ = new QWidget(central);
    auto *layout = new QHBoxLayout(editControls_);
    layout->setContentsMargins(0, 0, 0, 0);
    auto *sequence = new QVBoxLayout;
    auto *frameLabel = new QLabel(tr("Frames"));
    frames_ = new QListWidget;
    frames_->setObjectName("frameList");
    frames_->setAccessibleName(tr("Frame sequence"));
    frames_->setDragDropMode(QAbstractItemView::NoDragDrop);
    frames_->setMaximumWidth(160);
    sequence->addWidget(frameLabel);
    sequence->addWidget(frames_);
    auto addFrameButton = [this, sequence](const QString &text, auto command) {
        auto *button = new QPushButton(text);
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
    addFrameButton(tr("Delete"), [this] {
        apply(session_.removeFrame());
    })->setObjectName("deleteFrameButton");
    addFrameButton(tr("Move earlier"), [this] {
        const auto index = session_.selectedFrame();
        if(index > 0)
            apply(session_.moveFrame(index, index - 1));
    });
    addFrameButton(tr("Move later"), [this] {
        const auto index = session_.selectedFrame();
        if(index + 1 < session_.document().frameCount())
            apply(session_.moveFrame(index, index + 1));
    });
    layout->addLayout(sequence);
    grid_ = new GridWidget;
    layout->addWidget(grid_, 1);
    auto *side = new QVBoxLayout;
    inspector_ = new QGroupBox(tr("Selected cell"));
    inspector_->setObjectName("cellInspector");
    auto *inspectorLayout = new QVBoxLayout(inspector_);
    auto valueEditor = [this, inspectorLayout](
                           const QString &label, const char *name, int maximum,
                           QSpinBox *&spin, QSlider *&slider) {
        auto *caption = new QLabel(label);
        spin = new QSpinBox;
        spin->setObjectName(name);
        spin->setRange(0, maximum);
        spin->setKeyboardTracking(false);
        caption->setBuddy(spin);
        slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, maximum);
        slider->setAccessibleName(label);
        auto *row = new QHBoxLayout;
        auto *minimum = new QPushButton(tr("Min"));
        auto *maxButton = new QPushButton(tr("Max"));
        minimum->setAccessibleName(tr("Minimum %1").arg(label));
        maxButton->setAccessibleName(tr("Maximum %1").arg(label));
        row->addWidget(minimum);
        row->addWidget(spin);
        row->addWidget(maxButton);
        inspectorLayout->addWidget(caption);
        inspectorLayout->addLayout(row);
        inspectorLayout->addWidget(slider);
        connect(minimum, &QPushButton::clicked, spin,
                [spin] { spin->setValue(spin->minimum()); });
        connect(maxButton, &QPushButton::clicked, spin,
                [spin] { spin->setValue(spin->maximum()); });
        connect(spin, &QSpinBox::valueChanged, this, [this, slider](int value) {
            const QSignalBlocker blocker(slider);
            slider->setValue(value);
            editStimulus();
        });
        connect(slider, &QSlider::valueChanged, this, [this, spin](int value) {
            const QSignalBlocker blocker(spin);
            spin->setValue(value);
            editStimulus();
        });
        connect(slider, &QSlider::sliderPressed, this,
                [this] { session_.beginEditGroup(); });
        connect(slider, &QSlider::sliderReleased, this,
                [this] { session_.endEditGroup(); });
    };
    valueEditor(tr("&Frequency (Hz)"), "frequencySpinBox",
                static_cast<int>(maxFrequencyHz), frequency_, frequencySlider_);
    valueEditor(tr("&Amplitude (signed PCM peak)"), "amplitudeSpinBox",
                static_cast<int>(maxAmplitude), amplitude_, amplitudeSlider_);
    side->addWidget(inspector_);
    auto *settings = new QGroupBox(tr("Playback settings"));
    auto *form = new QFormLayout(settings);
    frameRate_ = new QSpinBox;
    frameRate_->setRange(1, 240);
    frameRate_->setObjectName("frameRateSpinBox");
    duration_ = new QDoubleSpinBox;
    duration_->setDecimals(3);
    duration_->setRange(0.001, 600);
    duration_->setSuffix(tr(" s"));
    sampleRate_ = new QComboBox;
    for(int rate : {8000, 16000, 22050, 44100, 48000, 96000, 192000})
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
    devices_->setAccessibleName(tr("Audio output device"));
    side->addWidget(new QLabel(tr("Output device")));
    side->addWidget(devices_);
    auto *reload = new QPushButton(tr("Refresh devices"));
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
    connect(frames_, &QListWidget::currentRowChanged, this, [this](int row) {
        if(row >= 0)
            apply(session_.selectFrame(static_cast<std::size_t>(row)));
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
        if(index >= 0)
            apply(backend_.selectDeviceId(
                devices_->itemData(index).toString().toStdString()));
    });
}

void EditorWindow::refresh()
{
    const bool playing = playback_.state() == PlaybackState::Playing;
    const bool busy = playing || exportState_ != nullptr;
    editControls_->setEnabled(!busy);
    for(auto *action : {new_, open_, import_, export_, routing_})
        action->setEnabled(!busy);
    undo_->setEnabled(!busy && session_.canUndo());
    redo_->setEnabled(!busy && session_.canRedo());
    play_->setEnabled(!exportState_);
    play_->setText(playing ? tr("Stop") : tr("Play"));
    const QSignalBlocker frameBlocker(frames_);
    if(static_cast<std::size_t>(frames_->count()) !=
       session_.document().frameCount())
    {
        frames_->clear();
        for(std::size_t i = 0; i < session_.document().frameCount(); ++i)
            frames_->addItem(tr("Frame %1").arg(i + 1));
    }
    frames_->setCurrentRow(static_cast<int>(session_.selectedFrame()));
    const auto settings = session_.document().settings();
    const QSignalBlocker rateBlocker(frameRate_), durationBlocker(duration_),
        sampleBlocker(sampleRate_), phaseBlocker(phase_);
    frameRate_->setValue(static_cast<int>(settings.frameRate));
    duration_->setValue(settings.durationMs / 1000.0);
    auto sampleIndex =
        sampleRate_->findData(static_cast<int>(settings.sampleRate));
    if(sampleIndex < 0)
    {
        sampleRate_->addItem(QString::number(settings.sampleRate),
                             static_cast<int>(settings.sampleRate));
        sampleIndex = sampleRate_->count() - 1;
    }
    sampleRate_->setCurrentIndex(sampleIndex);
    phase_->setCurrentIndex(phase_->findData(static_cast<int>(settings.phase)));
    inspector_->setEnabled(session_.selectedCell().has_value() && !busy);
    if(session_.selectedCell())
    {
        const auto value =
            session_.document()
                .cell(session_.selectedFrame(), *session_.selectedCell())
                .value();
        const QSignalBlocker f(frequency_), a(amplitude_), fs(frequencySlider_),
            as(amplitudeSlider_);
        const auto frequencyLimit = static_cast<int>(
            std::min(maxFrequencyHz, settings.sampleRate / 2 - 1));
        frequency_->setMaximum(frequencyLimit);
        frequencySlider_->setMaximum(frequencyLimit);
        frequency_->setValue(static_cast<int>(value.frequencyHz));
        amplitude_->setValue(static_cast<int>(value.amplitude));
        frequencySlider_->setValue(static_cast<int>(value.frequencyHz));
        amplitudeSlider_->setValue(static_cast<int>(value.amplitude));
        inspector_->setTitle(tr("Cell R%1 C%2")
                                 .arg(session_.selectedCell()->row + 1)
                                 .arg(session_.selectedCell()->column + 1));
    }
    else
        inspector_->setTitle(tr("Select a cell"));
    if(playing && playback_.snapshot())
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
    if(playing || exportState_)
    {
        if(!refreshTimer_.isActive())
            refreshTimer_.start();
    }
    else
        refreshTimer_.stop();
}

void EditorWindow::apply(Result<void> result)
{
    if(!result)
        showError(result.error());
    refresh();
}
void EditorWindow::showError(const Error &error)
{
    QMessageBox::critical(this, tr("Vibmory"),
                          QString::fromStdString(error.message));
}

void EditorWindow::editStimulus()
{
    if(!session_.selectedCell())
        return;
    apply(session_.editCell({static_cast<std::uint32_t>(frequency_->value()),
                             static_cast<std::uint32_t>(amplitude_->value())}));
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
    if(saveAs || path.empty())
    {
        auto selected = QFileDialog::getSaveFileName(
            this, tr("Save project"), toQString(path),
            tr("Vibmory project (*.vibmory)"));
        if(selected.isEmpty())
            return false;
        if(!selected.endsWith(".vibmory", Qt::CaseInsensitive))
            selected += ".vibmory";
        path = toPath(selected);
    }
    const auto result = store_.save(path, session_.document());
    if(!result)
    {
        showError(result.error());
        return false;
    }
    session_.markSaved(path);
    statusBar()->showMessage(tr("Project saved."), 3000);
    refresh();
    return true;
}

bool EditorWindow::confirmDiscard()
{
    if(!session_.dirty())
        return true;
    const auto choice = QMessageBox::warning(
        this, tr("Unsaved changes"), tr("Save changes before continuing?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);
    return choice == QMessageBox::Discard ||
           (choice == QMessageBox::Save && saveProject());
}

Result<void> EditorWindow::loadProject(const std::filesystem::path &path)
{
    auto result = store_.load(path);
    if(!result)
        return result.error();
    playback_.stop();
    session_.replace(std::move(result.value()), path);
    refresh();
    return {};
}

void EditorWindow::openProject(bool legacy)
{
    const auto selected = QFileDialog::getOpenFileName(
        this, legacy ? tr("Import legacy CSV") : tr("Open project"), {},
        legacy ? tr("Legacy CSV (*.csv)") : tr("Vibmory project (*.vibmory)"));
    if(selected.isEmpty())
        return;
    Result<PatternDocument> loaded = Error{ErrorCode::Cancelled, "Cancelled"};
    if(legacy)
    {
        bool accepted = false;
        const auto choice = QInputDialog::getItem(
            this, tr("Legacy grid orientation"),
            tr("The old writer and reader disagreed about dimensions. Select "
               "the header convention:"),
            {tr("Frame count, rows, columns (original writer)"),
             tr("Frame count, columns, rows (alternate)")},
            0, false, &accepted);
        if(!accepted)
            return;
        const auto orientation =
            choice == tr("Frame count, rows, columns (original writer)")
                ? LegacyOrientation::RowsThenColumns
                : LegacyOrientation::ColumnsThenRows;
        loaded = importLegacy(toPath(selected), orientation);
    }
    else
        loaded = store_.load(toPath(selected));
    if(!loaded)
    {
        showError(loaded.error());
        return;
    }
    if(!confirmDiscard())
        return;
    session_.replace(std::move(loaded.value()),
                     legacy ? std::filesystem::path{} : toPath(selected));
    refresh();
}

void EditorWindow::newProject()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("New project"));
    auto *layout = new QFormLayout(&dialog);
    auto *rows = new QSpinBox;
    rows->setRange(1, static_cast<int>(maxCells));
    rows->setValue(2);
    auto *columns = new QSpinBox;
    columns->setRange(1, static_cast<int>(maxCells));
    columns->setValue(2);
    layout->addRow(tr("Rows"), rows);
    layout->addRow(tr("Columns"), columns);
    auto *buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        auto result = PatternDocument::create(
            {static_cast<std::uint32_t>(rows->value()),
             static_cast<std::uint32_t>(columns->value())});
        if(!result)
            showError(result.error());
        else
            dialog.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if(dialog.exec() != QDialog::Accepted || !confirmDiscard())
        return;
    auto document =
        PatternDocument::create({static_cast<std::uint32_t>(rows->value()),
                                 static_cast<std::uint32_t>(columns->value())});
    session_.replace(std::move(document.value()));
    refresh();
}

void EditorWindow::togglePlayback()
{
    if(playback_.state() == PlaybackState::Playing)
        playback_.stop();
    else
    {
        const auto result = playback_.start(session_.snapshot());
        if(!result)
            showError(result.error());
    }
    refresh();
}

void EditorWindow::reportBackendFailure(const QString &message)
{
    if(playback_.state() != PlaybackState::Playing)
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
    const auto previous = devices_->currentData();
    const QSignalBlocker blocker(devices_);
    devices_->clear();
    for(const auto &device : backend_.devices())
        devices_->addItem(QString::fromStdString(device.label),
                          QString::fromStdString(device.id));
    const int index = devices_->findData(previous);
    if(index >= 0)
        devices_->setCurrentIndex(index);
    if(devices_->count() == 0)
        devices_->setPlaceholderText(
            tr("No audio output (WAV export available)"));
    else
        apply(backend_.selectDeviceId(
            devices_->currentData().toString().toStdString()));
}

void EditorWindow::exportWave()
{
    auto selected = QFileDialog::getSaveFileName(this, tr("Export WAV"), {},
                                                 tr("WAV audio (*.wav)"));
    if(selected.isEmpty())
        return;
    if(!selected.endsWith(".wav", Qt::CaseInsensitive))
        selected += ".wav";
    exportState_ = std::make_shared<ExportState>();
    const auto state = exportState_;
    exportDialog_ =
        new QProgressDialog(tr("Rendering audio…"), tr("Cancel"), 0, 100, this);
    exportDialog_->setWindowModality(Qt::WindowModal);
    exportDialog_->setAutoClose(false);
    connect(exportDialog_, &QProgressDialog::canceled, this,
            [state] { state->cancel = true; });
    auto snapshot = session_.snapshot();
    exportWatcher_.setFuture(
        QtConcurrent::run([state, path = toPath(selected),
                           document = std::move(snapshot.document)]() mutable {
            return exportWaveFile(path, std::move(document),
                                  [state](auto done, auto total) {
                                      state->done = done;
                                      state->total = total;
                                      return !state->cancel.load();
                                  });
        }));
    refresh();
}

void EditorWindow::configureRouting()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Physical channel routing and gains"));
    dialog.resize(560, 500);
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(tr("Each output channel maps to exactly one "
                                    "logical cell. Gains range from 0 to 1.")));
    const auto &document = session_.document();
    auto *table =
        new QTableWidget(static_cast<int>(document.shape().cellCount()), 3);
    table->setHorizontalHeaderLabels(
        {tr("Channel"), tr("Logical cell"), tr("Gain")});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    for(std::size_t i = 0; i < document.shape().cellCount(); ++i)
    {
        auto *channel = new QTableWidgetItem(QString::number(i + 1));
        channel->setFlags(Qt::ItemIsEnabled);
        table->setItem(static_cast<int>(i), 0, channel);
        auto *cell = new QComboBox;
        for(std::size_t j = 0; j < document.shape().cellCount(); ++j)
            cell->addItem(tr("R%1 C%2")
                              .arg(j / document.shape().columns + 1)
                              .arg(j % document.shape().columns + 1),
                          static_cast<int>(j));
        cell->setCurrentIndex(static_cast<int>(document.routing().cells[i]));
        auto *gain = new QDoubleSpinBox;
        gain->setDecimals(4);
        gain->setRange(0, 1);
        gain->setSingleStep(0.05);
        gain->setValue(document.routing().gains[i]);
        table->setCellWidget(static_cast<int>(i), 1, cell);
        table->setCellWidget(static_cast<int>(i), 2, gain);
    }
    layout->addWidget(table);
    auto *buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        auto routing = document.routing();
        for(std::size_t i = 0; i < routing.cells.size(); ++i)
        {
            routing.cells[i] = static_cast<std::size_t>(
                qobject_cast<QComboBox *>(
                    table->cellWidget(static_cast<int>(i), 1))
                    ->currentData()
                    .toUInt());
            routing.gains[i] = qobject_cast<QDoubleSpinBox *>(
                                   table->cellWidget(static_cast<int>(i), 2))
                                   ->value();
        }
        auto result = session_.setRouting(std::move(routing));
        if(!result)
            showError(result.error());
        else
            dialog.accept();
    });
    dialog.exec();
    refresh();
}

void EditorWindow::closeEvent(QCloseEvent *event)
{
    if(!confirmDiscard())
    {
        event->ignore();
        return;
    }
    playback_.stop();
    refreshTimer_.stop();
    if(exportState_)
        exportState_->cancel = true;
    exportWatcher_.waitForFinished();
    event->accept();
}
} // namespace vibmory
