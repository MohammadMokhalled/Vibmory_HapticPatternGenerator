#include "platform/project_store.h"
#include "platform/qt_audio_backend.h"
#include "ui/editor_window.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QTimer>

int main(int argc, char **argv)
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
    parser.addPositionalArgument("project",
                                 "Optional .vibmory project to open.");
    parser.process(application);
    vibmory::QtProjectStore store;
    vibmory::QtAudioBackend backend;
    vibmory::QtClock clock;
    auto initial = vibmory::PatternDocument::create({2, 2});
    if(!initial)
        return 1;
    vibmory::EditorWindow window(store, backend, clock,
                                 std::move(initial.value()));
    QObject::connect(&backend, &vibmory::QtAudioBackend::failed, &window,
                     &vibmory::EditorWindow::reportBackendFailure,
                     Qt::QueuedConnection);
    QObject::connect(&backend, &vibmory::QtAudioBackend::finished, &window,
                     &vibmory::EditorWindow::playbackFinished,
                     Qt::QueuedConnection);
    if(!parser.positionalArguments().isEmpty())
    {
        const auto result = window.loadProject(
            vibmory::toPath(parser.positionalArguments().first()));
        if(!result)
        {
            QMessageBox::critical(
                &window, "Vibmory",
                QString::fromStdString(result.error().message));
            return 1;
        }
    }
    window.show();
    if(parser.isSet("smoke-test"))
        QTimer::singleShot(300, &application, &QCoreApplication::quit);
    return application.exec();
}
