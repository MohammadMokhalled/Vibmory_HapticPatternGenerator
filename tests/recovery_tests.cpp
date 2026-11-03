#include "platform/project_store.h"
#include "platform/recovery_store.h"
#include "platform/text_file.h"
#include "ui/recovery_controller.h"

#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QPushButton>
#include <QTemporaryDir>
#include <QtTest>

#include <atomic>
#include <cstdlib>
#include <stdexcept>
#include <thread>

using namespace vibmory;

namespace
{
PatternDocument document()
{
    auto result = PatternDocument::create({1, 2});
    auto document = std::move(result.value());
    (void)document.setCell(0, {0, 1}, {150, 2000});
    return document;
}

void clickRecoveryButton(const char* name)
{
    auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
    QVERIFY(dialog);
    QCOMPARE(dialog->objectName(), QString("recoveryDialog"));
    auto* button = dialog->findChild<QPushButton*>(name);
    QVERIFY(button && button->isEnabled());
    button->click();
}
} // namespace

class RecoveryTests final : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        QApplication::setQuitOnLastWindowClosed(false);
    }

    void roundTripLockAndExplicitDiscard()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        auto created = RecoveryFile::create(temporary.path());
        QVERIFY(created);
        auto file = std::move(created.value());
        const auto path = file->path();
        const auto original = toPath(temporary.filePath("project-ä.vibmory"));
        const RecoveryRecord expected{document(), original,
                                      QDateTime::currentDateTimeUtc()};
        QVERIFY(writeRecoveryFile(toPath(path), expected, {}));
        auto loaded = readRecoveryFile(toPath(path));
        QVERIFY(loaded);
        QVERIFY(loaded.value().document == expected.document);
        QCOMPARE(loaded.value().originalPath, original);
        QCOMPARE(loaded.value().capturedAt, expected.capturedAt);
        auto active = RecoveryFile::available(temporary.path());
        QVERIFY(active && active.value().empty());
        // Releasing a lease simulates termination without clean discard.
        file.reset();
        auto available = RecoveryFile::available(temporary.path());
        QVERIFY(available && available.value().size() == 1);
        QVERIFY(QFile::exists(path));
        auto competing = RecoveryFile::available(temporary.path());
        QVERIFY(competing && competing.value().empty());
        QVERIFY(available.value().front()->discard());
        QVERIFY(!QFile::exists(path));
    }

    void terminatedProcessLeavesARecoverableSnapshotAndStaleLock()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        QProcess child;
        child.start(
            QCoreApplication::applicationFilePath(),
            {"-platform", "offscreen", "--leave-recovery", temporary.path()});
        QVERIFY(child.waitForFinished(10000));
        QCOMPARE(child.exitCode(), 17);
        const auto files =
            QDir(temporary.path()).entryList({"*.vibrecovery"}, QDir::Files);
        QCOMPARE(files.size(), qsizetype{1});
        const auto path = QDir(temporary.path()).filePath(files.front());
        QVERIFY(QFile::exists(path + ".lock"));
        auto recovered = RecoveryFile::available(temporary.path());
        QVERIFY(recovered && recovered.value().size() == 1);
        QVERIFY(readRecoveryFile(toPath(recovered.value().front()->path()))
                    .value()
                    .document == document());
        QVERIFY(recovered.value().front()->discard());
        recovered.value().clear();
        QVERIFY(!QFile::exists(path + ".lock"));
    }

    void cancellationAndMalformedInputPreserveTheLastGoodSnapshot()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const auto path = toPath(temporary.filePath("record.vibrecovery"));
        const RecoveryRecord expected{
            document(), {}, QDateTime::currentDateTimeUtc()};
        QVERIFY(writeRecoveryFile(path, expected, {}));
        const auto original = readTextFile(path, 1024 * 1024).value();
        const auto cancelled =
            writeRecoveryFile(path, expected, [] { return false; });
        QVERIFY(!cancelled && cancelled.error().code == ErrorCode::Cancelled);
        QCOMPARE(readTextFile(path, 1024 * 1024).value(), original);
        auto bad = expected;
        bad.capturedAt = {};
        QVERIFY(!writeRecoveryFile(path, bad, {}));
        QCOMPARE(readTextFile(path, 1024 * 1024).value(), original);
        for (const auto* content :
             {"", "VIBMORY_RECOVERY 999\n{}\n", "VIBMORY_RECOVERY 1\n{\n",
              "VIBMORY_RECOVERY "
              "1\n{\"originalPath\":\"relative.vibmory\",\"capturedAt\":\"2026-"
              "10-24T12:00:00.000Z\"}\n"})
        {
            QVERIFY(writeTextFileAtomically(path, content));
            QVERIFY(!readRecoveryFile(path));
        }
        QVERIFY(writeTextFileAtomically(
            path, original.substr(0, original.size() / 2)));
        QVERIFY(!readRecoveryFile(path));
        QFile oversized(toQString(path));
        QVERIFY(oversized.open(QIODevice::WriteOnly));
        QVERIFY(oversized.resize(17 * 1024 * 1024));
        oversized.close();
        QVERIFY(!readRecoveryFile(path));
    }

    void retentionIsBoundedAndNeverPrunesAnActiveInstance()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const auto now = QDateTime::currentDateTimeUtc();
        const RecoveryRecord record{document(), {}, now};
        QString activePath;
        std::unique_ptr<RecoveryFile> active;
        for (int index = 0; index < 13; ++index)
        {
            auto created = RecoveryFile::create(temporary.path());
            QVERIFY(created);
            auto file = std::move(created.value());
            QVERIFY(writeRecoveryFile(toPath(file->path()), record, {}));
            QFile dated(file->path());
            QVERIFY(dated.open(QIODevice::ReadWrite));
            QVERIFY(dated.setFileTime(now.addSecs(-index * 60),
                                      QFileDevice::FileModificationTime));
            dated.close();
            if (index == 12)
            {
                activePath = file->path();
                active = std::move(file);
            }
        }
        auto retained = RecoveryFile::available(temporary.path(), now);
        QVERIFY(retained && retained.value().size() == 10);
        QCOMPARE(QDir(temporary.path())
                     .entryList({"*.vibrecovery"}, QDir::Files)
                     .size(),
                 qsizetype{11});
        QVERIFY(QFile::exists(activePath));
        retained.value().clear();
        auto expired =
            RecoveryFile::available(temporary.path(), now.addDays(31));
        QVERIFY(expired && expired.value().empty());
        QVERIFY(QFile::exists(activePath));
        active.reset();
        expired = RecoveryFile::available(temporary.path(), now.addDays(31));
        QVERIFY(expired && expired.value().empty());
        QVERIFY(!QFile::exists(activePath));
    }

    void backgroundSnapshotsAreImmutableAndDoNotOverwriteTheProject()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const auto directory = temporary.filePath("recovery");
        const auto originalPath = toPath(temporary.filePath("project.vibmory"));
        QtProjectStore store;
        QVERIFY(store.save(originalPath, document()));
        EditorSession session(document());
        session.markSaved(originalPath);
        QVERIFY(session.selectCell(CellPosition{0, 0}));
        QVERIFY(session.editCell({100, 500}));
        const auto firstRevision = session.revision();
        auto release = std::make_shared<std::atomic<bool>>(false);
        RecoveryController recovery(
            session, directory,
            [release](const auto& path, const auto& record,
                      const auto& proceed) {
                while (!release->load() && proceed())
                    std::this_thread::yield();
                return writeRecoveryFile(path, record, proceed);
            });
        QSignalSpy saved(&recovery, &RecoveryController::snapshotSaved);
        recovery.captureNow();
        QVERIFY(recovery.busy());
        QVERIFY(session.editCell({200, 1000}));
        release->store(true);
        QTRY_COMPARE(saved.count(), 1);
        QCOMPARE(saved.front().at(0).toULongLong(),
                 static_cast<qulonglong>(firstRevision));
        const auto paths =
            QDir(directory).entryList({"*.vibrecovery"}, QDir::Files);
        QCOMPARE(paths.size(), qsizetype{1});
        const auto path = toPath(QDir(directory).filePath(paths.front()));
        QCOMPARE(readRecoveryFile(path)
                     .value()
                     .document.cell(0, {0, 0})
                     .value()
                     .frequencyHz,
                 100U);
        recovery.captureNow();
        QTRY_COMPARE(saved.count(), 2);
        QCOMPARE(readRecoveryFile(path)
                     .value()
                     .document.cell(0, {0, 0})
                     .value()
                     .frequencyHz,
                 200U);
        recovery.captureNow();
        QVERIFY(!recovery.busy());
        QVERIFY(store.load(originalPath).value() == document());
        session.markSaved(originalPath);
        recovery.observe();
        QVERIFY(!QFile::exists(toQString(path)));
    }

    void cancellationRestartExceptionsAndReplacementHaveNoStalePublication()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        EditorSession session(document());
        QVERIFY(session.addFrame());
        auto hold = std::make_shared<std::atomic<bool>>(true);
        auto fail = std::make_shared<std::atomic<bool>>(false);
        RecoveryController recovery(
            session, temporary.path(),
            [hold, fail](const auto& path, const auto& record,
                         const auto& proceed) {
                if (fail->load())
                    throw std::runtime_error("test recovery exception");
                while (hold->load() && proceed())
                    std::this_thread::yield();
                return writeRecoveryFile(path, record, proceed);
            });
        QSignalSpy saved(&recovery, &RecoveryController::snapshotSaved);
        QSignalSpy warning(&recovery, &RecoveryController::warningOccurred);
        recovery.captureNow();
        recovery.clear();
        hold->store(false);
        recovery.captureNow();
        QTRY_COMPARE(saved.count(), 1);
        QCoreApplication::processEvents();
        QCOMPARE(saved.count(), 1);
        session.replace(document());
        recovery.observe();
        QVERIFY(QDir(temporary.path())
                    .entryList({"*.vibrecovery"}, QDir::Files)
                    .isEmpty());
        QVERIFY(session.addFrame());
        fail->store(true);
        recovery.captureNow();
        QTRY_COMPARE(warning.count(), 1);
        QVERIFY(!recovery.busy());
        fail->store(false);
        recovery.captureNow();
        QTRY_COMPARE(saved.count(), 2);
        recovery.clear();
        QVERIFY(QDir(temporary.path())
                    .entryList({"*.vibrecovery"}, QDir::Files)
                    .isEmpty());
    }

    void startupKeepRestoreAndDiscardAreExplicit()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        auto created = RecoveryFile::create(temporary.path());
        QVERIFY(created);
        auto file = std::move(created.value());
        const auto path = file->path();
        const auto original = toPath(temporary.filePath("original.vibmory"));
        QVERIFY(writeRecoveryFile(
            toPath(path),
            {document(), original, QDateTime::currentDateTimeUtc()}, {}));
        file.reset();
        EditorSession session(PatternDocument::create({2, 2}).value());
        RecoveryController recovery(session, temporary.path());
        QTimer::singleShot(0,
                           [] { clickRecoveryButton("keepRecoveryButton"); });
        QVERIFY(!recovery.offerRestore(nullptr));
        QVERIFY(QFile::exists(path) && !session.dirty());
        QTimer::singleShot(
            0, [] { clickRecoveryButton("restoreRecoveryButton"); });
        QVERIFY(recovery.offerRestore(nullptr));
        QVERIFY(session.dirty() && session.document() == document());
        QCOMPARE(session.path(), original);
        auto competing = RecoveryFile::available(temporary.path());
        QVERIFY(competing && competing.value().empty());
        recovery.clear();
        QVERIFY(!QFile::exists(path));
        // Corrupt snapshots remain discardable, without becoming a document.
        created = RecoveryFile::create(temporary.path());
        QVERIFY(created);
        file = std::move(created.value());
        const auto corruptPath = file->path();
        QVERIFY(writeTextFileAtomically(toPath(corruptPath), "corrupt"));
        file.reset();
        session.replace(document());
        QTimer::singleShot(
            0, [] { clickRecoveryButton("discardRecoveryButton"); });
        QVERIFY(!recovery.offerRestore(nullptr));
        QVERIFY(!QFile::exists(corruptPath) && !session.dirty());
    }
};

int main(int argc, char** argv)
{
    QApplication application(argc, argv);
    const auto arguments = application.arguments();
    const auto childFlag = arguments.indexOf("--leave-recovery");
    if (childFlag >= 0 && childFlag + 1 < arguments.size())
    {
        auto file = RecoveryFile::create(arguments[childFlag + 1]);
        if (!file || !writeRecoveryFile(
                         toPath(file.value()->path()),
                         {document(), {}, QDateTime::currentDateTimeUtc()}, {}))
            return 2;
        // Deliberately bypass destructors to exercise stale-lock recovery.
        std::_Exit(17);
    }
    RecoveryTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "recovery_tests.moc"
