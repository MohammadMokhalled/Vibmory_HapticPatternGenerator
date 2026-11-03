#include "ui/ui_preferences.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QLibraryInfo>
#include <QMenu>
#include <QResource>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>

#include <utility>

// This explicit reference keeps the translation resource in static UI builds.
static void initializeTranslations()
{
    static const bool initialized = [] {
        Q_INIT_RESOURCE(vibmory_translations);
        return true;
    }();
    Q_UNUSED(initialized);
}

namespace vibmory
{
namespace
{
QString themeName(ColorTheme theme)
{
    switch (theme)
    {
    case ColorTheme::Light:
        return "light";
    case ColorTheme::Dark:
        return "dark";
    case ColorTheme::System:
        return "system";
    }
    return {};
}

bool validLanguage(const QString& language)
{
    return language == "system" || language == "en" || language == "de";
}

QPalette paletteFor(bool dark)
{
    QPalette palette;
    const QColor window(dark ? "#181f2b" : "#f5f7fb");
    const QColor base(dark ? "#111722" : "#ffffff");
    const QColor text(dark ? "#edf2fa" : "#172235");
    const QColor button(dark ? "#253147" : "#e8eef7");
    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase,
                     QColor(dark ? "#202a3b" : "#eaf0f8"));
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::PlaceholderText,
                     QColor(dark ? "#b0bfd4" : "#596b85"));
    palette.setColor(QPalette::Button, button);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, Qt::white);
    palette.setColor(QPalette::Highlight, QColor(dark ? "#73a6ff" : "#2457c5"));
    palette.setColor(QPalette::HighlightedText,
                     QColor(dark ? "#0b182e" : "#ffffff"));
    palette.setColor(QPalette::Link, QColor(dark ? "#8ab5ff" : "#285ac6"));
    palette.setColor(QPalette::LinkVisited,
                     QColor(dark ? "#c1a6ff" : "#6840a8"));
    palette.setColor(QPalette::Light, QColor(dark ? "#43536e" : "#ffffff"));
    palette.setColor(QPalette::Midlight, QColor(dark ? "#34425a" : "#d9e1ee"));
    palette.setColor(QPalette::Mid, QColor(dark ? "#52627a" : "#8393a9"));
    palette.setColor(QPalette::Dark, QColor(dark ? "#0b1019" : "#5d6d81"));
    palette.setColor(QPalette::Shadow, QColor(dark ? "#070a10" : "#526078"));
    palette.setColor(QPalette::ToolTipBase,
                     QColor(dark ? "#2c3a52" : "#ffffe1"));
    palette.setColor(QPalette::ToolTipText, text);
    const QColor disabled(dark ? "#8c9bb0" : "#718096");
    for (auto role :
         {QPalette::Text, QPalette::WindowText, QPalette::ButtonText})
        palette.setColor(QPalette::Disabled, role, disabled);
    palette.setColor(QPalette::Disabled, QPalette::Highlight,
                     QColor(dark ? "#35435b" : "#d0daeb"));
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabled);
    return palette;
}
} // namespace

UiPreferences::UiPreferences(QSettings& settings, QObject* parent)
    : QObject(parent), settings_(settings), originalLocale_(),
      originalPalette_(QApplication::palette()),
      originalStyle_(QApplication::style()->objectName())
{
    initializeTranslations();
}

UiPreferences::~UiPreferences()
{
    if (!initialized_)
        return;
    QCoreApplication::removeTranslator(&translator_);
    QCoreApplication::removeTranslator(&qtTranslator_);
    QLocale::setDefault(originalLocale_);
    if (auto* style = QStyleFactory::create(originalStyle_))
        QApplication::setStyle(style);
    QApplication::setPalette(originalPalette_);
}

Result<void> UiPreferences::initialize(const QString& themeOverride,
                                       const QString& languageOverride)
{
    if (initialized_)
        return invalid("UI preferences are already initialized.");
    if ((!themeOverride.isEmpty() && themeOverride != "system" &&
         themeOverride != "light" && themeOverride != "dark") ||
        (!languageOverride.isEmpty() && !validLanguage(languageOverride)))
        return invalid(
            "Expected theme system/light/dark and language system/en/de.");
    const auto theme =
        themeOverride.isEmpty()
            ? settings_.value("appearance/theme", "system").toString()
            : themeOverride;
    theme_ = theme == "dark"    ? ColorTheme::Dark
             : theme == "light" ? ColorTheme::Light
                                : ColorTheme::System;
    language_ = languageOverride.isEmpty()
                    ? settings_.value("language/ui", "system").toString()
                    : languageOverride;
    if (!validLanguage(language_))
        language_ = "system";
    appliedLanguage_ =
        language_ == "system"
            ? (QLocale::system().language() == QLocale::German ? "de" : "en")
            : language_;
    if (appliedLanguage_ == "de")
    {
        if (!translator_.load(":/i18n/vibmory_de.qm"))
            return Error{ErrorCode::Io,
                         "The German application translation is unavailable."};
        // CI/SDK builds use Qt's installation path; deployed builds use the
        // adjacent translations directory selected by qt.conf.
        if (!qtTranslator_.load(
                "qt_de", QLibraryInfo::path(QLibraryInfo::TranslationsPath)) &&
            !qtTranslator_.load("qt_de",
                                QCoreApplication::applicationDirPath() +
                                    "/translations"))
            return Error{ErrorCode::Io,
                         "The German Qt dialog translation is unavailable."};
        QCoreApplication::installTranslator(&qtTranslator_);
        QCoreApplication::installTranslator(&translator_);
    }
    QLocale::setDefault(language_ == "system" ? QLocale::system()
                                              : QLocale(appliedLanguage_));
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    initialized_ = true;
    applyTheme();
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, [this] {
                if (theme_ == ColorTheme::System)
                    applyTheme();
            });
    return {};
}

void UiPreferences::applyTheme()
{
    const bool dark =
        theme_ == ColorTheme::Dark ||
        (theme_ == ColorTheme::System &&
         QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark);
    QApplication::setPalette(paletteFor(dark));
    emit themeChanged();
}

Result<void> UiPreferences::persist(const QString& key, const QString& value)
{
    settings_.setValue(key, value);
    settings_.sync();
    if (settings_.status() != QSettings::NoError)
        return Error{ErrorCode::Io, "Cannot save application preferences."};
    return {};
}

Result<void> UiPreferences::setTheme(ColorTheme theme)
{
    if (!initialized_ || themeName(theme).isEmpty())
        return invalid("Unknown theme or uninitialized UI preferences.");
    if (auto result = persist("appearance/theme", themeName(theme)); !result)
        return result;
    theme_ = theme;
    applyTheme();
    return {};
}

Result<void> UiPreferences::setLanguage(const QString& language)
{
    if (!initialized_ || !validLanguage(language))
        return invalid("Unsupported language or uninitialized UI preferences.");
    if (auto result = persist("language/ui", language); !result)
        return result;
    language_ = language;
    emit notice(tr("Language changes take effect after restarting Vibmory."));
    return {};
}

void UiPreferences::populateMenu(QMenu& menu)
{
    auto* themes = menu.addMenu(tr("&Theme"));
    auto* themeGroup = new QActionGroup(themes);
    const std::pair<const char*, ColorTheme> choices[] = {
        {"system", ColorTheme::System},
        {"light", ColorTheme::Light},
        {"dark", ColorTheme::Dark}};
    for (const auto& [name, value] : choices)
    {
        const auto label = value == ColorTheme::System  ? tr("Follow system")
                           : value == ColorTheme::Light ? tr("Light")
                                                        : tr("Dark");
        auto* action = themes->addAction(label);
        action->setObjectName(QString("theme_%1").arg(name));
        action->setCheckable(true);
        action->setChecked(theme_ == value);
        themeGroup->addAction(action);
        connect(action, &QAction::triggered, this, [this, value, themeGroup] {
            if (const auto result = setTheme(value); !result)
            {
                for (auto* item : themeGroup->actions())
                    item->setChecked(item->objectName() ==
                                     "theme_" + themeName(theme_));
                emit notice(QString::fromStdString(result.error().message));
            }
        });
    }
    auto* languages = menu.addMenu(tr("&Language (next launch)"));
    auto* languageGroup = new QActionGroup(languages);
    for (const auto& language :
         {QString("system"), QString("en"), QString("de")})
    {
        auto* action =
            languages->addAction(language == "system" ? tr("System language")
                                 : language == "en"   ? QString("English")
                                                      : QString("Deutsch"));
        action->setObjectName("language_" + language);
        action->setCheckable(true);
        action->setChecked(language_ == language);
        languageGroup->addAction(action);
        connect(
            action, &QAction::triggered, this, [this, language, languageGroup] {
                if (const auto result = setLanguage(language); !result)
                {
                    for (auto* item : languageGroup->actions())
                        item->setChecked(item->objectName() ==
                                         "language_" + language_);
                    emit notice(QString::fromStdString(result.error().message));
                }
            });
    }
}
} // namespace vibmory
