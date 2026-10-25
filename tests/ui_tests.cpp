#include "audio/pcm_renderer.h"
#include "persistence/project_codec.h"
#include "platform/device_profile_store.h"
#include "platform/project_store.h"
#include "platform/recovery_store.h"
#include "platform/text_file.h"
#include "platform/wave_export_job.h"
#include "ui/cell_inspector.h"
#include "ui/device_profile_actions.h"
#include "ui/editor_window.h"
#include "ui/frame_list_model.h"
#include "ui/grid_widget.h"
#include "ui/ui_preferences.h"

#include <QAbstractItemModelTester>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QMimeData>
#include <QPointer>
#include <QPushButton>
#include <QScopeGuard>
#include <QSettings>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

#include <atomic>
#include <memory>
#include <stdexcept>
#include <thread>

using namespace vibmory;

namespace
{
PatternDocument document()
{
    auto result = PatternDocument::create({2, 3});
    auto value = std::move(result.value());
    (void)value.setCell(0, {1, 2}, {150, 1000});
    (void)value.addFrame();
    (void)value.setCell(1, {1, 2}, {250, 2000});
    return value;
}

class Store final : public DocumentStore
{
public:
    bool fail = false;
    bool failWrite = false;
    mutable int saves = 0;
    Result<PatternDocument> load(const std::filesystem::path&) const override
    {
        if (fail)
            return Error{ErrorCode::Io, "test read failure"};
        return document();
    }
    Result<void> save(const std::filesystem::path&,
                      const PatternDocument&) const override
    {
        ++saves;
        if (failWrite)
            return Error{ErrorCode::Io, "test write failure"};
        return {};
    }
};
class Backend final : public PlaybackBackend
{
public:
    bool fail = false;
    int stops = 0;
    int selections = 0;
    std::string selected;
    std::vector<AudioDeviceDescription> choices;
    std::vector<AudioDeviceDescription> devices() const override
    {
        return choices;
    }
    std::string selectedDeviceId() const override
    {
        return selected;
    }
    Result<void> selectDeviceId(const std::string& id) override
    {
        selected = id;
        ++selections;
        return {};
    }
    Result<void> start(const DocumentSnapshot&) override
    {
        if (fail)
            return Error{ErrorCode::Device, "unsupported test device"};
        return {};
    }
    void stop() noexcept override
    {
        ++stops;
    }
    std::optional<std::uint64_t> processedSampleFrames() const override
    {
        return 0;
    }
};
class Clock final : public MonotonicClock
{
public:
    std::uint64_t microseconds() const override
    {
        return 0;
    }
};
} // namespace

class UiTests final : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        QApplication::setQuitOnLastWindowClosed(false);
    }

    void rectangularGeometry()
    {
        const QRectF bounds(0, 0, 301, 203);
        const GridShape shape{2, 3};
        QCOMPARE(GridGeometry::cellAt(bounds, shape, {300, 202}).value(),
                 (CellPosition{1, 2}));
        QCOMPARE(GridGeometry::cellAt(bounds, shape, {101, 1}).value(),
                 (CellPosition{0, 1}));
        QVERIFY(!GridGeometry::cellAt(bounds, shape, {301, 0}));
        QVERIFY(!GridGeometry::cellAt({}, shape, {0, 0}));
        QCOMPARE(GridGeometry::cellAt({0, 0, 1, 1}, shape, {0.9, 0.9}).value(),
                 (CellPosition{1, 2}));
    }

    void inspectorRefreshDoesNotEmitCommands()
    {
        CellInspector inspector;
        QSignalSpy edits(&inspector, &CellInspector::stimulusEdited);
        inspector.setCell(CellPosition{0, 0}, CellStimulus{150, 1000},
                          maxFrequencyHz);
        inspector.setCell(CellPosition{1, 2}, CellStimulus{0, 0},
                          maxFrequencyHz);
        QCOMPARE(edits.count(), 0);
        inspector.findChild<QSpinBox*>("amplitudeSpinBox")->setValue(123);
        QCOMPARE(edits.count(), 1);
        QVERIFY(inspector.stimulus() == (CellStimulus{0, 123}));
    }

    void keyboardAndMouseSelectionRespectBounds()
    {
        const auto source = document();
        GridWidget grid;
        grid.setFrame(source, 0, {});
        QSignalSpy selections(&grid, &GridWidget::cellSelected);
        QSignalSpy cleared(&grid, &GridWidget::selectionCleared);
        connect(
            &grid, &GridWidget::cellSelected, &grid, [&](int row, int column) {
                grid.setFrame(source, 0,
                              CellPosition{static_cast<std::uint32_t>(row),
                                           static_cast<std::uint32_t>(column)});
            });
        grid.show();
        grid.setFocus();
        QTest::keyClick(&grid, Qt::Key_Right);
        QTest::keyClick(&grid, Qt::Key_Down);
        QTest::keyClick(&grid, Qt::Key_Right);
        QTest::keyClick(&grid, Qt::Key_Right);
        QCOMPARE(selections.last().at(0).toInt(), 1);
        QCOMPARE(selections.last().at(1).toInt(), 2);
        const auto count = selections.count();
        QTest::mouseClick(&grid, Qt::RightButton, Qt::NoModifier, QPoint(1, 1));
        QCOMPARE(selections.count(), count);
        QTest::keyClick(&grid, Qt::Key_Escape);
        QCOMPARE(cleared.count(), 1);
    }

    void presentationPreferencesDoNotEditOrResetTheSession()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        QSettings settings(temporary.filePath("preferences.ini"),
                           QSettings::IniFormat);
        UiPreferences preferences(settings);
        QVERIFY(preferences.initialize("light", "en"));
        Store store;
        Backend backend;
        Clock clock;
        WaveExportJob exporter;
        EditorWindow window(store, backend, clock, exporter, document());
        window.enablePreferences(preferences);
        auto* grid = window.findChild<GridWidget*>("gridWidget");
        grid->cellSelected(1, 2);
        const auto before = window.session().document();
        const auto revision = window.session().revision();
        auto* timeline = window.findChild<QListView*>("frameList");
        const auto thumbnail = timeline->model()
                                   ->index(0, 0)
                                   .data(Qt::DecorationRole)
                                   .value<QIcon>()
                                   .cacheKey();
        auto* dark = window.findChild<QAction*>("theme_dark");
        QVERIFY(dark);
        dark->trigger();
        QCOMPARE(preferences.theme(), ColorTheme::Dark);
        QTRY_VERIFY(timeline->model()
                        ->index(0, 0)
                        .data(Qt::DecorationRole)
                        .value<QIcon>()
                        .cacheKey() != thumbnail);
        const auto icon = window.findChild<QPushButton*>("playButton")
                              ->icon()
                              .pixmap(32, 32)
                              .toImage();
        QCOMPARE(icon.pixelColor(16, 16),
                 window.palette().color(QPalette::ButtonText));
        auto* german = window.findChild<QAction*>("language_de");
        QVERIFY(german);
        german->trigger();
        QCOMPARE(preferences.language(), QString("de"));
        QCOMPARE(preferences.appliedLanguage(), QString("en"));
        QVERIFY(window.session().document() == before);
        QCOMPARE(window.session().revision(), revision);
        QVERIFY(window.session().selectedCell() == (CellPosition{1, 2}));
        QVERIFY(!window.session().dirty());
        QVERIFY(!window.findChild<QPushButton*>("playButton")->icon().isNull());
        QVERIFY(!window.findChild<QPushButton*>("moveEarlierButton")
                     ->icon()
                     .isNull());
    }

    void deviceRefreshDoesNotSwitchThePhysicalOutput()
    {
        Store store;
        Backend backend;
        Clock clock;
        WaveExportJob exporter;
        backend.choices = {{"first", "First device"},
                           {"second", "Chosen device"}};
        backend.selected = "second";
        EditorWindow window(store, backend, clock, exporter, document());
        auto* devices = window.findChild<QComboBox*>("outputDeviceComboBox");
        QCOMPARE(devices->currentData().toString(), QString("second"));
        QCOMPARE(backend.selections, 0);
        window.findChild<QPushButton*>("refreshDevicesButton")->click();
        QCOMPARE(backend.selected, std::string("second"));
        QCOMPARE(backend.selections, 0);
        devices->setCurrentIndex(0);
        QCOMPARE(backend.selected, std::string("first"));
        QCOMPARE(backend.selections, 1);
    }

    void selectionRefreshDoesNotEdit()
    {
        Store store;
        Backend backend;
        Clock clock;
        WaveExportJob exporter;
        EditorWindow window(store, backend, clock, exporter, document());
        window.show();
        QCoreApplication::processEvents();
        auto* inspector = window.findChild<QGroupBox*>("cellInspector");
        QVERIFY(inspector && !inspector->isEnabled());
        auto* grid = window.findChild<GridWidget*>("gridWidget");
        const auto bounds =
            GridGeometry::cellRect(QRectF(grid->rect()), {2, 3}, {1, 2});
        QTest::mouseClick(grid, Qt::LeftButton, Qt::NoModifier,
                          bounds.center().toPoint());
        QVERIFY(window.session().selectedCell() == (CellPosition{1, 2}));
        QCOMPARE(window.findChild<QSpinBox*>("frequencySpinBox")->value(), 150);
        QCOMPARE(window.session().revision(), std::uint64_t{0});
        QVERIFY(!window.session().dirty());
        auto* list = window.findChild<QListView*>("frameList");
        list->setCurrentIndex(list->model()->index(1, 0));
        QVERIFY(!window.session().selectedCell());
        QCOMPARE(window.session().revision(), std::uint64_t{0});
        QVERIFY(!window.findChild<QGroupBox*>("cellInspector")->isEnabled());
    }

    void frameCommandsStaySynchronized()
    {
        Store store;
        Backend backend;
        Clock clock;
        WaveExportJob exporter;
        EditorWindow window(store, backend, clock, exporter, document());
        auto* list = window.findChild<QListView*>("frameList");
        window.findChild<QPushButton*>("duplicateFrameButton")->click();
        QCOMPARE(window.session().document().frameCount(), std::size_t{3});
        QCOMPARE(list->currentIndex().row(), 1);
        QCOMPARE(
            window.session().document().cell(2, {1, 2}).value().frequencyHz,
            250U);
        window.findChild<QPushButton*>("deleteFrameButton")->click();
        QCOMPARE(window.session().document().frameCount(), std::size_t{2});
        QCOMPARE(list->model()->rowCount(), 2);
        QCOMPARE(
            window.session().document().cell(1, {1, 2}).value().frequencyHz,
            250U);
    }

    void removingFirstOrLastKeepsTheIntendedFrame()
    {
        for (int removed : {0, 1})
        {
            Store store;
            Backend backend;
            Clock clock;
            WaveExportJob exporter;
            EditorWindow window(store, backend, clock, exporter, document());
            auto* list = window.findChild<QListView*>("frameList");
            list->setCurrentIndex(list->model()->index(removed, 0));
            auto* remove = window.findChild<QPushButton*>("deleteFrameButton");
            remove->click();
            QCOMPARE(window.session().document().frameCount(), std::size_t{1});
            QCOMPARE(
                window.session().document().cell(0, {1, 2}).value().frequencyHz,
                removed == 0 ? 250U : 150U);
            QVERIFY(!remove->isEnabled());
        }
    }

    void timelineReorderPreservesIdentitySelectionAndUndo()
    {
        Store store;
        Backend backend;
        Clock clock;
        WaveExportJob exporter;
        EditorWindow window(store, backend, clock, exporter, document());
        auto* list = window.findChild<QListView*>("frameList");
        auto* model = qobject_cast<FrameListModel*>(list->model());
        QVERIFY(model);
        QAbstractItemModelTester tester(
            model, QAbstractItemModelTester::FailureReportingMode::QtTest);
        const auto id = window.session().document().frameId(0);
        const auto preview =
            model->index(0).data(Qt::DecorationRole).value<QIcon>();
        QVERIFY(!preview.isNull());
        QSignalSpy moved(model, &QAbstractItemModel::rowsMoved);
        std::unique_ptr<QMimeData> drag(model->mimeData({model->index(0)}));
        QVERIFY(model->dropMimeData(drag.get(), Qt::MoveAction, 2, 0, {}));
        QCOMPARE(moved.count(), 1);
        QCOMPARE(window.session().selectedFrame(), std::size_t{1});
        QCOMPARE(list->currentIndex().row(), 1);
        QCOMPARE(window.session().document().frameId(1), id);
        QCOMPARE(model->index(1).data(Qt::UserRole).toULongLong(),
                 static_cast<qulonglong>(id));
        QVERIFY(window.session().dirty());
        const auto saved =
            decodeProject(encodeProject(window.session().document()));
        QVERIFY(saved && saved.value().frameId(1) == id);
        // A drag captured before an edit must never act on a new revision.
        QVERIFY(!model->dropMimeData(drag.get(), Qt::MoveAction, 0, 0, {}));
        for (auto* action : window.findChildren<QAction*>())
            if (action->shortcut() == QKeySequence(QKeySequence::Undo))
                action->trigger();
        QCOMPARE(window.session().document().frameId(0), id);
        QCOMPARE(list->currentIndex().row(), 0);
        QVERIFY(!window.session().dirty());
        QCOMPARE(model->index(0)
                     .data(Qt::DecorationRole)
                     .value<QIcon>()
                     .pixmap(80, 48)
                     .toImage(),
                 preview.pixmap(80, 48).toImage());
    }

    void timelineRejectsForeignMalformedAndRejectedCommands()
    {
        auto source = document();
        int requests = 0;
        FrameListModel model([&](auto, auto, auto) {
            ++requests;
            return false;
        });
        model.setDocument(source, 10);
        QAbstractItemModelTester tester(
            &model, QAbstractItemModelTester::FailureReportingMode::QtTest);
        FrameListModel foreign({}, nullptr);
        foreign.setDocument(source, 10);
        std::unique_ptr<QMimeData> foreignDrag(
            foreign.mimeData({foreign.index(0)}));
        QVERIFY(
            !model.dropMimeData(foreignDrag.get(), Qt::MoveAction, 2, 0, {}));
        QMimeData malformed;
        malformed.setData("application/x-vibmory-frame", QByteArray(101, 'x'));
        QVERIFY(!model.canDropMimeData(&malformed, Qt::MoveAction, 2, 0, {}));
        std::unique_ptr<QMimeData> drag(model.mimeData({model.index(0)}));
        QVERIFY(!model.dropMimeData(drag.get(), Qt::CopyAction, 2, 0, {}));
        QVERIFY(!model.moveRows({}, 0, 1, {}, 1));
        QVERIFY(!model.moveRows({}, 0, 2, {}, 2));
        QCOMPARE(requests, 0);
        QSignalSpy moved(&model, &QAbstractItemModel::rowsMoved);
        QVERIFY(!model.dropMimeData(drag.get(), Qt::MoveAction, 2, 0, {}));
        QCOMPARE(requests, 1);
        QCOMPARE(moved.count(), 0);
        QCOMPARE(model.index(0).data(Qt::UserRole).toULongLong(),
                 static_cast<qulonglong>(source.frameId(0)));
        model.setDocument(source, 11);
        QVERIFY(!model.dropMimeData(drag.get(), Qt::MoveAction, 2, 0, {}));
        QCOMPARE(requests, 1);
    }

    void timelineRefreshOnlyInvalidatesChangedThumbnails()
    {
        auto source = document();
        FrameListModel model({});
        model.setDocument(source, 0);
        QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
        QSignalSpy reset(&model, &QAbstractItemModel::modelReset);
        const auto unchanged =
            model.index(1).data(Qt::DecorationRole).value<QIcon>().cacheKey();
        const auto edited =
            model.index(0).data(Qt::DecorationRole).value<QIcon>().cacheKey();
        model.setDocument(source, 0);
        QCOMPARE(changed.count(), 0);
        QCOMPARE(reset.count(), 0);
        QVERIFY(source.setCell(0, {0, 0}, {200, 5000}));
        model.setDocument(source, 1);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(reset.count(), 0);
        QCOMPARE(
            model.index(1).data(Qt::DecorationRole).value<QIcon>().cacheKey(),
            unchanged);
        QVERIFY(
            model.index(0).data(Qt::DecorationRole).value<QIcon>().cacheKey() !=
            edited);
    }

    void paintingIsPure()
    {
        Store store;
        Backend backend;
        Clock clock;
        WaveExportJob exporter;
        EditorWindow window(store, backend, clock, exporter, document());
        window.show();
        QCoreApplication::processEvents();
        const auto before = window.session().document();
        auto* grid = window.findChild<GridWidget*>("gridWidget");
        const auto image = grid->grab();
        QVERIFY(!image.isNull());
        window.resize(900, 650);
        grid->repaint();
        QCoreApplication::processEvents();
        QVERIFY(window.session().document() == before);
        QCOMPARE(window.session().revision(), std::uint64_t{0});
    }

    void failedLoadPreservesWorkingDocument()
    {
        Store store;
        Backend backend;
        Clock clock;
        WaveExportJob exporter;
        EditorWindow window(store, backend, clock, exporter, document());
        const auto before = window.session().document();
        store.fail = true;
        QVERIFY(!window.loadProject("missing.vibmory"));
        QVERIFY(window.session().document() == before);
    }

    void restoredWindowCloseClearsItsOwnRecoveryFile()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        auto created = RecoveryFile::create(temporary.path());
        QVERIFY(created);
        auto file = std::move(created.value());
        const auto path = file->path();
        QVERIFY(writeRecoveryFile(
            toPath(path), {document(), {}, QDateTime::currentDateTimeUtc()},
            {}));
        file.reset();
        Store store;
        Backend backend;
        Clock clock;
        WaveExportJob exporter;
        EditorWindow window(store, backend, clock, exporter, document());
        QTimer::singleShot(0, [] {
            auto* dialog =
                qobject_cast<QDialog*>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            auto* restore =
                dialog->findChild<QPushButton*>("restoreRecoveryButton");
            QVERIFY(restore);
            restore->click();
        });
        window.enableRecovery(temporary.path(), true);
        QVERIFY(window.session().dirty());
        QTimer::singleShot(0, [] {
            auto* dialog =
                qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            dialog->button(QMessageBox::Discard)->click();
        });
        QVERIFY(window.close());
        QVERIFY(!QFile::exists(path));
    }

    void deployedEditorCanSaveReopenAndExport_data()
    {
        QTest::addColumn<unsigned>("rows");
        QTest::addColumn<unsigned>("columns");
        QTest::newRow("2x3") << 2U << 3U;
        QTest::newRow("3x2") << 3U << 2U;
    }

    void deployedEditorCanSaveReopenAndExport()
    {
        QFETCH(unsigned, rows);
        QFETCH(unsigned, columns);
        // This case also runs natively against only the deployed Qt DLLs and
        // plugins. The fake output keeps hardware availability independent.
        const bool previous =
            QApplication::testAttribute(Qt::AA_DontUseNativeDialogs);
        QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
        const auto restore = qScopeGuard([previous] {
            QApplication::setAttribute(Qt::AA_DontUseNativeDialogs, previous);
        });
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        QtProjectStore store;
        Backend backend;
        Clock clock;
        WaveExportJob exporter;
        auto created = PatternDocument::create({rows, columns});
        QVERIFY(created);
        auto source = std::move(created.value());
        QVERIFY(source.addFrame());
        QVERIFY(source.setSettings({7, 300, 8000}));
        for (std::size_t frame = 0; frame < source.frameCount(); ++frame)
            for (unsigned row = 0; row < rows; ++row)
                for (unsigned column = 0; column < columns; ++column)
                {
                    const auto index = row * columns + column + 1;
                    QVERIFY(source.setCell(
                        frame, {row, column},
                        {100 * index,
                         200 * index + static_cast<unsigned>(frame)}));
                }
        EditorWindow window(store, backend, clock, exporter, source);
        window.show();
        auto* grid = window.findChild<GridWidget*>("gridWidget");
        QCoreApplication::processEvents();
        for (unsigned row = 0; row < rows; ++row)
            for (unsigned column = 0; column < columns; ++column)
            {
                const auto bounds = GridGeometry::cellRect(
                    QRectF(grid->rect()), {rows, columns}, {row, column});
                QTest::mouseClick(grid, Qt::LeftButton, Qt::NoModifier,
                                  bounds.center().toPoint());
                QVERIFY(window.session().selectedCell() ==
                        (CellPosition{row, column}));
                QCOMPARE(
                    window.findChild<QSpinBox*>("frequencySpinBox")->value(),
                    static_cast<int>(100 * (row * columns + column + 1)));
            }
        window.findChild<QSpinBox*>("amplitudeSpinBox")->setValue(3210);
        QVERIFY(window.session().dirty());
        const auto expected = window.session().document();
        auto triggerWithPath = [&](const QKeySequence& shortcut,
                                   const QString& path) -> bool {
            QAction* action = nullptr;
            for (auto* candidate : window.findChildren<QAction*>())
                if (candidate->shortcut() == shortcut)
                    action = candidate;
            if (!action)
                return false;
            bool accepted = false;
            bool timedOut = false;
            QElapsedTimer elapsed;
            elapsed.start();
            QTimer responder;
            connect(&responder, &QTimer::timeout, &window, [&] {
                if (elapsed.elapsed() > 5000)
                {
                    timedOut = true;
                    if (auto* modal = qobject_cast<QDialog*>(
                            QApplication::activeModalWidget()))
                        modal->reject();
                    return;
                }
                if (auto* dialog = qobject_cast<QFileDialog*>(
                        QApplication::activeModalWidget()))
                {
                    // Qt's selectFile deliberately ignores the focused name
                    // editor after showing. Enter the filename as a user would.
                    dialog->setDirectory(QFileInfo(path).absolutePath());
                    auto* name = dialog->findChild<QLineEdit*>("fileNameEdit");
                    if (!name)
                        return;
                    name->setText(QFileInfo(path).fileName());
                    accepted = QMetaObject::invokeMethod(dialog, "accept",
                                                         Qt::DirectConnection);
                }
            });
            responder.start(10);
            action->trigger();
            responder.stop();
            return accepted && !timedOut;
        };
        const auto base = temporary.filePath(QString::fromUtf8("pattern-ä"));
        const auto project = base + ".vibmory";
        QVERIFY(triggerWithPath(QKeySequence(QKeySequence::Save), base));
        QVERIFY(!window.session().dirty());
        QVERIFY(window.session().path() == toPath(project));
        auto saved = store.load(toPath(project));
        QVERIFY(saved);
        QVERIFY(saved.value() == expected);
        const auto other = temporary.filePath("other.vibmory");
        auto different = PatternDocument::create({1, 1});
        QVERIFY(different);
        QVERIFY(store.save(toPath(other), different.value()));
        QVERIFY(window.loadProject(toPath(other)));
        QVERIFY(triggerWithPath(QKeySequence(QKeySequence::Open), project));
        QVERIFY(window.session().document() == expected);
        QVERIFY(!window.session().dirty());
        const auto wave = base + ".wav";
        QSignalSpy completed(&exporter, &WaveExportJob::finished);
        QVERIFY(triggerWithPath(QKeySequence("Ctrl+E"), base));
        QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 10000);
        QVERIFY(completed.first().first().value<Result<void>>());
        QFile exported(wave);
        QVERIFY(exported.open(QIODevice::ReadOnly));
        QByteArray expectedBytes;
        QVERIFY(
            renderWave(expected, [&expectedBytes](auto bytes) -> Result<void> {
                expectedBytes.append(
                    reinterpret_cast<const char*>(bytes.data()),
                    static_cast<qsizetype>(bytes.size()));
                return {};
            }));
        QCOMPARE(exported.readAll(), expectedBytes);
        QVERIFY(window.close());
    }

    void atomicSaveAndCancelledExport()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        QtProjectStore store;
        const auto path = toPath(temporary.filePath("project.vibmory"));
        const auto expected = document();
        QVERIFY(store.save(path, expected));
        auto loaded = store.load(path);
        QVERIFY(loaded && loaded.value() == expected);
        QVERIFY(!store.save(toPath(temporary.path()), expected));
        const auto wavePath = temporary.filePath("output.wav");
        QFile previous(wavePath);
        QVERIFY(previous.open(QIODevice::WriteOnly));
        QCOMPARE(previous.write("previous"), qint64{8});
        previous.close();
        const auto cancelled = exportWaveFile(toPath(wavePath), expected,
                                              [](auto, auto) { return false; });
        QVERIFY(!cancelled && cancelled.error().code == ErrorCode::Cancelled);
        QVERIFY(previous.open(QIODevice::ReadOnly));
        QCOMPARE(previous.readAll(), QByteArray("previous"));
    }

    void deviceProfileReviewRequiresExplicitOutputSelection()
    {
        const auto source = document();
        DeviceProfile profile{"<b>Calibration</b>", source.shape(),
                              source.routing(), "preferred"};
        const std::vector<AudioDeviceDescription> devices{
            {"preferred", "Preferred output"}};
        QTimer::singleShot(0, [] {
            auto* dialog =
                qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            QCOMPARE(dialog->objectName(),
                     QString("deviceProfileReviewDialog"));
            QCOMPARE(dialog->textFormat(), Qt::PlainText);
            QCOMPARE(dialog->defaultButton(),
                     dialog->button(QMessageBox::Cancel));
            QVERIFY(dialog->checkBox() && !dialog->checkBox()->isChecked());
            QVERIFY(dialog->detailedText().contains("Channel 1"));
            dialog->checkBox()->setChecked(true);
            dialog->button(QMessageBox::Apply)->click();
        });
        const auto selected =
            DeviceProfileActions::reviewProfile(profile, devices, nullptr);
        QVERIFY(selected && *selected);
        QTimer::singleShot(0, [] {
            auto* dialog =
                qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            dialog->button(QMessageBox::Cancel)->click();
        });
        QVERIFY(
            !DeviceProfileActions::reviewProfile(profile, devices, nullptr));
        QTimer::singleShot(0, [] {
            auto* dialog =
                qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            QVERIFY(dialog && !dialog->checkBox());
            dialog->button(QMessageBox::Apply)->click();
        });
        const auto retained =
            DeviceProfileActions::reviewProfile(profile, {}, nullptr);
        QVERIFY(retained && !*retained);
    }

    void deviceProfileRoundTripAndRejectedSavePreserveExistingFile()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const auto path =
            toPath(temporary.filePath("calibration-ä.vibprofile"));
        const auto source = document();
        DeviceProfile profile{"Kuue kanaliga – kalibreeritud", source.shape(),
                              source.routing(), "aabbcc"};
        profile.routing.gains[2] = 0.123456789012345;
        std::swap(profile.routing.cells[0], profile.routing.cells[5]);
        QVERIFY(saveDeviceProfile(path, profile));
        const auto loaded = loadDeviceProfile(path);
        QVERIFY(loaded && loaded.value() == profile);
        const auto original = readTextFile(path, maxDeviceProfileBytes).value();
        profile.routing.gains[2] = 2;
        QVERIFY(!saveDeviceProfile(path, profile));
        QCOMPARE(readTextFile(path, maxDeviceProfileBytes).value(), original);
        profile.routing.gains[2] = 0.5;
        profile.name = std::string(1, static_cast<char>(0xff));
        QVERIFY(!saveDeviceProfile(path, profile));
        QCOMPARE(readTextFile(path, maxDeviceProfileBytes).value(), original);
        QVERIFY(!saveDeviceProfile(toPath(temporary.path()), loaded.value()));
    }

    void deviceProfileReaderRejectsMalformedSchemaAndOversizedInput()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const auto path = toPath(temporary.filePath("profile.vibprofile"));
        const auto source = document();
        QVERIFY(saveDeviceProfile(
            path, {"Calibration", source.shape(), source.routing(), {}}));
        const auto original =
            QJsonDocument::fromJson(
                QByteArray::fromStdString(
                    readTextFile(path, maxDeviceProfileBytes).value()))
                .object();
        auto rejects = [&](const QJsonObject& object) {
            return writeTextFileAtomically(
                       path, QJsonDocument(object).toJson().toStdString()) &&
                   !loadDeviceProfile(path);
        };
        for (const auto& version :
             {QJsonValue(2), QJsonValue(true), QJsonValue("1")})
        {
            auto object = original;
            object["version"] = version;
            QVERIFY(rejects(object));
        }
        auto object = original;
        object["extra"] = 1;
        QVERIFY(rejects(object));
        object = original;
        object["grid"] = QJsonObject{{"rows", 2.5}, {"columns", 3}};
        QVERIFY(rejects(object));
        for (const auto& cell : {QJsonValue(-1), QJsonValue(1.5),
                                 QJsonValue(true), QJsonValue(500)})
        {
            object = original;
            auto channels = object["channels"].toArray();
            channels[0] = QJsonObject{{"cell", cell}, {"gain", 1}};
            object["channels"] = channels;
            QVERIFY(rejects(object));
        }
        object = original;
        auto channels = object["channels"].toArray();
        channels[0] = channels[1];
        object["channels"] = channels;
        QVERIFY(rejects(object));
        QVERIFY(writeTextFileAtomically(
            path, std::string(maxDeviceProfileBytes + 1, 'x')));
        QVERIFY(!loadDeviceProfile(path));
        QVERIFY(writeTextFileAtomically(path, "{truncated"));
        QVERIFY(!loadDeviceProfile(path));
    }

    void asyncCancellationPreservesTheExistingFile()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const auto output = temporary.filePath("cancelled.wav");
        QFile previous(output);
        QVERIFY(previous.open(QIODevice::WriteOnly));
        QCOMPARE(previous.write("previous"), qint64{8});
        previous.close();
        auto source = document();
        QVERIFY(source.setSettings({10, 600000, 48000}));
        WaveExportJob job;
        QSignalSpy completed(&job, &WaveExportJob::finished);
        QVERIFY(job.start(toPath(output), {source, 1}));
        QVERIFY(job.busy());
        job.cancel();
        QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 5000);
        const auto result = completed.takeFirst().at(0).value<Result<void>>();
        QVERIFY(!result && result.error().code == ErrorCode::Cancelled);
        QVERIFY(!job.busy());
        QVERIFY(previous.open(QIODevice::ReadOnly));
        QCOMPARE(previous.readAll(), QByteArray("previous"));
    }

    void workerExceptionsBecomeRecoverableErrors()
    {
        WaveExportJob job([](const std::filesystem::path&, PatternDocument,
                             const RenderProgress&) -> Result<void> {
            throw std::runtime_error("test worker failure");
        });
        QSignalSpy completed(&job, &WaveExportJob::finished);
        QVERIFY(job.start("unused.wav", {document(), 1}));
        QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 5000);
        const auto result = completed.takeFirst().at(0).value<Result<void>>();
        QVERIFY(!result && result.error().code == ErrorCode::Io);
        QVERIFY(result.error().message.find("test worker failure") !=
                std::string::npos);
        QVERIFY(!job.busy());
    }

    void aCancelledJobCanRestartWithoutStaleCompletion()
    {
        const auto gate = std::make_shared<std::atomic<bool>>(false);
        WaveExportJob job(
            [gate](const std::filesystem::path&, PatternDocument,
                   const RenderProgress& progress) -> Result<void> {
                while (!gate->load())
                {
                    if (!progress(0, 1))
                        return Error{ErrorCode::Cancelled, "cancelled"};
                    std::this_thread::yield();
                }
                return {};
            });
        QSignalSpy completed(&job, &WaveExportJob::finished);
        QVERIFY(job.start("unused.wav", {document(), 1}));
        job.cancelAndWait();
        gate->store(true);
        QVERIFY(job.start("unused.wav", {document(), 2}));
        QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 5000);
        QVERIFY(completed.takeFirst().at(0).value<Result<void>>());
        QVERIFY(!job.busy());
    }

    void cancelClosePreservesEdits()
    {
        Store store;
        Backend backend;
        Clock clock;
        WaveExportJob exporter;
        EditorWindow window(store, backend, clock, exporter, document());
        window.show();
        QCoreApplication::processEvents();
        auto* grid = window.findChild<GridWidget*>("gridWidget");
        QTest::mouseClick(grid, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
        window.findChild<QSpinBox*>("amplitudeSpinBox")->setValue(123);
        QVERIFY(window.session().dirty());
        QTimer::singleShot(10, [] {
            if (auto* box = qobject_cast<QMessageBox*>(
                    QApplication::activeModalWidget()))
                box->button(QMessageBox::Cancel)->click();
        });
        QVERIFY(!window.close());
        QVERIFY(window.isVisible());
        QCOMPARE(window.session().document().cell(0, {0, 0}).value().amplitude,
                 123U);
    }

    void failedSavePreventsClose()
    {
        Store store;
        Backend backend;
        Clock clock;
        WaveExportJob exporter;
        EditorWindow window(store, backend, clock, exporter, document());
        QVERIFY(window.loadProject("saved.vibmory"));
        window.show();
        QCoreApplication::processEvents();
        auto* grid = window.findChild<GridWidget*>("gridWidget");
        QTest::mouseClick(grid, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
        window.findChild<QSpinBox*>("amplitudeSpinBox")->setValue(123);
        store.failWrite = true;
        QTimer::singleShot(10, [] {
            if (auto* box = qobject_cast<QMessageBox*>(
                    QApplication::activeModalWidget()))
                box->button(QMessageBox::Save)->click();
            QTimer::singleShot(10, [] {
                if (auto* box = qobject_cast<QMessageBox*>(
                        QApplication::activeModalWidget()))
                    box->button(QMessageBox::Ok)->click();
            });
        });
        QVERIFY(!window.close());
        QVERIFY(window.session().dirty());
        QCOMPARE(store.saves, 1);
    }

    void failedPlaybackRestoresControls()
    {
        Store store;
        Backend backend;
        Clock clock;
        backend.fail = true;
        WaveExportJob exporter;
        EditorWindow window(store, backend, clock, exporter, document());
        QTimer::singleShot(10, [] {
            if (auto* box = qobject_cast<QMessageBox*>(
                    QApplication::activeModalWidget()))
                box->button(QMessageBox::Ok)->click();
        });
        window.findChild<QPushButton*>("playButton")->click();
        QCOMPARE(window.findChild<QPushButton*>("playButton")->text(),
                 QString("Play"));
        QVERIFY(window.findChild<QListView*>("frameList")->isEnabled());
        QVERIFY(!window.session().dirty());
    }

    void closingWhileExportingCancelsAndJoinsTheWorker()
    {
        WaveExportJob exporter(
            [](const std::filesystem::path&, PatternDocument,
               const RenderProgress& progress) -> Result<void> {
                while (progress(0, 1))
                    std::this_thread::yield();
                return Error{ErrorCode::Cancelled, "cancelled"};
            });
        Store store;
        Backend backend;
        Clock clock;
        EditorWindow window(store, backend, clock, exporter, document());
        QVERIFY(exporter.start("unused.wav", {document(), 1}));
        window.show();
        QVERIFY(window.close());
        QVERIFY(!exporter.busy());
    }

    void repeatedWindowDestructionReleasesChildWidgets()
    {
        Store store;
        Backend backend;
        Clock clock;
        WaveExportJob exporter;
        for (int iteration = 0; iteration < 10; ++iteration)
        {
            auto window = std::make_unique<EditorWindow>(store, backend, clock,
                                                         exporter, document());
            QPointer<GridWidget> grid =
                window->findChild<GridWidget*>("gridWidget");
            QSignalSpy destroyed(window.get(), &QObject::destroyed);
            window.reset();
            QCOMPARE(destroyed.count(), 1);
            QVERIFY(grid.isNull());
        }
    }

    void playbackRestoresEditingWithoutMutatingSelection()
    {
        Store store;
        Backend backend;
        Clock clock;
        WaveExportJob exporter;
        EditorWindow window(store, backend, clock, exporter, document());
        auto* list = window.findChild<QListView*>("frameList");
        list->setCurrentIndex(list->model()->index(1, 0));
        auto* button = window.findChild<QPushButton*>("playButton");
        button->click();
        QCOMPARE(button->text(), QString("Stop"));
        QVERIFY(!list->isEnabled());
        QTest::qWait(40);
        QCOMPARE(window.session().selectedFrame(), std::size_t{1});
        button->click();
        QCOMPARE(button->text(), QString("Play"));
        QCOMPARE(window.session().selectedFrame(), std::size_t{1});
        QVERIFY(!window.session().dirty());
    }
};

QTEST_MAIN(UiTests)
#include "ui_tests.moc"
