#include "platform/project_store.h"
#include "ui/editor_window.h"
#include "ui/grid_widget.h"

#include <QFile>
#include <QGroupBox>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QtTest>

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
    Result<PatternDocument> load(const std::filesystem::path &) const override
    {
        if(fail)
            return Error{ErrorCode::Io, "test read failure"};
        return document();
    }
    Result<void> save(const std::filesystem::path &,
                      const PatternDocument &) const override
    {
        return {};
    }
};
class Backend final : public PlaybackBackend
{
        public:
    Result<void> start(const DocumentSnapshot &) override
    {
        return {};
    }
    void stop() noexcept override
    {
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

    void selectionRefreshDoesNotEdit()
    {
        Store store;
        Backend backend;
        Clock clock;
        EditorWindow window(store, backend, clock, document());
        window.show();
        QCoreApplication::processEvents();
        auto *inspector = window.findChild<QGroupBox *>("cellInspector");
        QVERIFY(inspector && !inspector->isEnabled());
        auto *grid = window.findChild<GridWidget *>("gridWidget");
        const auto bounds =
            GridGeometry::cellRect(QRectF(grid->rect()), {2, 3}, {1, 2});
        QTest::mouseClick(grid, Qt::LeftButton, Qt::NoModifier,
                          bounds.center().toPoint());
        QVERIFY(window.session().selectedCell() == (CellPosition{1, 2}));
        QCOMPARE(window.findChild<QSpinBox *>("frequencySpinBox")->value(),
                 150);
        QCOMPARE(window.session().revision(), std::uint64_t{0});
        QVERIFY(!window.session().dirty());
        auto *list = window.findChild<QListWidget *>("frameList");
        list->setCurrentRow(1);
        QVERIFY(!window.session().selectedCell());
        QCOMPARE(window.session().revision(), std::uint64_t{0});
        QVERIFY(!window.findChild<QGroupBox *>("cellInspector")->isEnabled());
    }

    void frameCommandsStaySynchronized()
    {
        Store store;
        Backend backend;
        Clock clock;
        EditorWindow window(store, backend, clock, document());
        auto *list = window.findChild<QListWidget *>("frameList");
        window.findChild<QPushButton *>("duplicateFrameButton")->click();
        QCOMPARE(window.session().document().frameCount(), std::size_t{3});
        QCOMPARE(list->currentRow(), 1);
        QCOMPARE(
            window.session().document().cell(2, {1, 2}).value().frequencyHz,
            250U);
        window.findChild<QPushButton *>("deleteFrameButton")->click();
        QCOMPARE(window.session().document().frameCount(), std::size_t{2});
        QCOMPARE(list->count(), 2);
        QCOMPARE(
            window.session().document().cell(1, {1, 2}).value().frequencyHz,
            250U);
    }

    void paintingIsPure()
    {
        Store store;
        Backend backend;
        Clock clock;
        EditorWindow window(store, backend, clock, document());
        window.show();
        QCoreApplication::processEvents();
        const auto before = window.session().document();
        auto *grid = window.findChild<GridWidget *>("gridWidget");
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
        EditorWindow window(store, backend, clock, document());
        const auto before = window.session().document();
        store.fail = true;
        QVERIFY(!window.loadProject("missing.vibmory"));
        QVERIFY(window.session().document() == before);
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

    void playbackRestoresEditingWithoutMutatingSelection()
    {
        Store store;
        Backend backend;
        Clock clock;
        EditorWindow window(store, backend, clock, document());
        auto *list = window.findChild<QListWidget *>("frameList");
        list->setCurrentRow(1);
        auto *button = window.findChild<QPushButton *>("playButton");
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
