#include "platform/project_store.h"
#include "platform/qt_audio_backend.h"
#include "platform/wave_export_job.h"
#include "ui/editor_window.h"
#include "ui/ui_preferences.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QMessageBox>
#include <QPixmap>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>

#include <memory>

int main(int argc, char** argv)
{
    QApplication application(argc, argv);
    QCoreApplication::setApplicationName("Vibmory");
    QCoreApplication::setApplicationVersion(VIBMORY_VERSION);
    QCoreApplication::setOrganizationName("Vibmory");
    QCommandLineParser parser;
    parser.setApplicationDescription("Haptic pattern editor");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(
        {"smoke-test",
         "Launch the complete UI and exit automatically without audio."});
    parser.addOption({"screenshot",
                      "Save an editor preview and exit without audio.",
                      "path"});
    parser.addOption(
        {"no-recovery",
         "Disable recovery snapshots and startup recovery prompts."});
    parser.addOption({"theme",
                      "Override this launch's theme: system, light, dark.",
                      "theme"});
    parser.addOption({"language",
                      "Override this launch's UI language: system, en, de.",
                      "language"});
    parser.addPositionalArgument("project",
                                 "Optional .vibmory project to open.");
    parser.process(application);
    const bool automated =
        parser.isSet("smoke-test") || parser.isSet("screenshot");
    std::unique_ptr<QTemporaryDir> isolatedSettings;
    std::unique_ptr<QSettings> settings;
    if (automated)
    {
        isolatedSettings = std::make_unique<QTemporaryDir>();
        if (!isolatedSettings->isValid())
            return 1;
        settings = std::make_unique<QSettings>(
            isolatedSettings->filePath("preferences.ini"),
            QSettings::IniFormat);
    }
    else
        settings = std::make_unique<QSettings>();
    vibmory::UiPreferences preferences(*settings);
    if (const auto result = preferences.initialize(parser.value("theme"),
                                                   parser.value("language"));
        !result)
    {
        if (automated)
        {
            qCritical().noquote()
                << QString::fromStdString(result.error().message);
            return 1;
        }
        QMessageBox::critical(nullptr, "Vibmory",
                              QString::fromStdString(result.error().message));
        return 1;
    }
    vibmory::QtProjectStore store;
    vibmory::QtAudioBackend backend;
    vibmory::QtClock clock;
    vibmory::WaveExportJob exporter;
    auto initial = vibmory::PatternDocument::create({2, 2});
    if (!initial)
        return 1;
    vibmory::EditorWindow window(store, backend, clock, exporter,
                                 std::move(initial.value()));
    window.enablePreferences(preferences);
    QObject::connect(
        &backend, &vibmory::QtAudioBackend::failed, &window,
        [&backend, &window](const QString& message, quint64 generation) {
            if (generation == backend.generation())
                window.reportBackendFailure(message);
        },
        Qt::QueuedConnection);
    QObject::connect(
        &backend, &vibmory::QtAudioBackend::finished, &window,
        [&backend, &window](quint64 generation) {
            if (generation == backend.generation())
                window.playbackFinished();
        },
        Qt::QueuedConnection);
    if (!parser.positionalArguments().isEmpty())
    {
        const auto result = window.loadProject(
            vibmory::toPath(parser.positionalArguments().first()));
        if (!result)
        {
            QMessageBox::critical(
                &window, "Vibmory",
                QString::fromStdString(result.error().message));
            return 1;
        }
    }
    if (!automated && !parser.isSet("no-recovery"))
    {
        const auto data = QStandardPaths::writableLocation(
            QStandardPaths::AppLocalDataLocation);
        if (!data.isEmpty())
            window.enableRecovery(data + "/recovery",
                                  parser.positionalArguments().isEmpty());
    }
    window.show();
    if (parser.isSet("smoke-test"))
        QTimer::singleShot(300, &application, &QCoreApplication::quit);
    else if (parser.isSet("screenshot"))
        QTimer::singleShot(300, &application, [&] {
            application.exit(
                window.grab().save(parser.value("screenshot")) ? 0 : 1);
        });
    return application.exec();
}
