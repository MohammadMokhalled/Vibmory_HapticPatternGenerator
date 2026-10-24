#include "persistence/project_codec.h"
#include "ui/grid_widget.h"
#include "ui/ui_preferences.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QSettings>
#include <QStyleHints>
#include <QTemporaryDir>
#include <QtTest>

#include <algorithm>
#include <cmath>
#include <utility>

using namespace vibmory;

namespace
{
double luminance(QColor color)
{
    const auto linear = [](double value) {
        return value <= 0.04045 ? value / 12.92
                                : std::pow((value + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) +
           0.0722 * linear(color.blueF());
}

double contrast(QColor a, QColor b)
{
    const auto first = luminance(a), second = luminance(b);
    return (std::max(first, second) + 0.05) / (std::min(first, second) + 0.05);
}
} // namespace

class PreferencesTests final : public QObject
{
    Q_OBJECT
private slots:
    void themeColorsHaveReadableActiveTextAndPersist()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        QSettings settings(temporary.filePath("preferences.ini"),
                           QSettings::IniFormat);
        UiPreferences preferences(settings);
        QVERIFY(preferences.initialize("light", "en"));
        QSignalSpy changed(&preferences, &UiPreferences::themeChanged);
        for (const auto theme : {ColorTheme::Dark, ColorTheme::Light})
        {
            QVERIFY(preferences.setTheme(theme));
            const auto palette = QApplication::palette();
            for (const auto& pair :
                 {std::pair{QPalette::WindowText, QPalette::Window},
                  {QPalette::Text, QPalette::Base},
                  {QPalette::ButtonText, QPalette::Button},
                  {QPalette::HighlightedText, QPalette::Highlight},
                  {QPalette::ToolTipText, QPalette::ToolTipBase}})
                QVERIFY(contrast(palette.color(pair.first),
                                 palette.color(pair.second)) >= 4.5);
        }
        QCOMPARE(changed.count(), 2);
        QCOMPARE(settings.value("appearance/theme").toString(),
                 QString("light"));
        QVERIFY(preferences.setTheme(ColorTheme::System));
        QCOMPARE(preferences.theme(), ColorTheme::System);
        const bool systemDark = QGuiApplication::styleHints()->colorScheme() ==
                                Qt::ColorScheme::Dark;
        QCOMPARE(QApplication::palette().color(QPalette::Window).lightness() <
                     128,
                 systemDark);
        QVERIFY(!preferences.setTheme(static_cast<ColorTheme>(99)));
        QCOMPARE(preferences.theme(), ColorTheme::System);
    }

    void stimulusColorsRetainReadableNumericValues()
    {
        for (std::uint32_t frequency = 0; frequency <= maxFrequencyHz;
             frequency += 100)
            for (std::uint32_t amplitude = 0; amplitude <= maxAmplitude;
                 amplitude += 1024)
                QVERIFY(contrast(Qt::black,
                                 stimulusColor({frequency, amplitude})) >= 4.5);
    }

    void languageChangesApplyOnNextInitializationAndPreserveProjectEncoding()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const auto path = temporary.filePath("preferences.ini");
        auto document = PatternDocument::create({1, 1}).value();
        QVERIFY(document.setRouting({{0}, {0.25}}));
        const auto original = encodeProject(document);
        {
            QSettings settings(path, QSettings::IniFormat);
            UiPreferences preferences(settings);
            QVERIFY(preferences.initialize("light", "en"));
            QVERIFY(preferences.setLanguage("de"));
            QCOMPARE(preferences.appliedLanguage(), QString("en"));
            QCOMPARE(
                QCoreApplication::translate("vibmory::EditorWindow", "&File"),
                QString("&File"));
        }
        {
            QSettings settings(path, QSettings::IniFormat);
            UiPreferences preferences(settings);
            QVERIFY(preferences.initialize());
            QCOMPARE(preferences.appliedLanguage(), QString("de"));
            QCOMPARE(
                QCoreApplication::translate("vibmory::EditorWindow", "&File"),
                QString("&Datei"));
            QCOMPARE(QCoreApplication::translate("vibmory::CellInspector",
                                                 "Select a cell"),
                     QString("Zelle auswählen"));
            QCOMPARE(QLocale().decimalPoint(), QString(","));
            QDialogButtonBox buttons(QDialogButtonBox::Cancel);
            QCOMPARE(buttons.button(QDialogButtonBox::Cancel)->text(),
                     QString("Abbrechen"));
            QCOMPARE(encodeProject(document), original);
            QVERIFY(decodeProject(original).value() == document);
            QVERIFY(!preferences.setLanguage("unknown"));
            QCOMPARE(preferences.language(), QString("de"));
        }
    }

    void invalidSavedPreferencesUseDefaultsAndOverridesDoNotWrite()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        QSettings settings(temporary.filePath("preferences.ini"),
                           QSettings::IniFormat);
        settings.setValue("appearance/theme", "invalid");
        settings.setValue("language/ui", "unsupported");
        UiPreferences preferences(settings);
        QVERIFY(!preferences.initialize("invalid", "en"));
        QVERIFY(preferences.initialize({}, "en"));
        QCOMPARE(preferences.theme(), ColorTheme::System);
        QCOMPARE(preferences.appliedLanguage(), QString("en"));
        QCOMPARE(settings.value("appearance/theme").toString(),
                 QString("invalid"));
        QCOMPARE(settings.value("language/ui").toString(),
                 QString("unsupported"));
        QVERIFY(!preferences.initialize());
    }
};

QTEST_MAIN(PreferencesTests)
#include "preferences_tests.moc"
