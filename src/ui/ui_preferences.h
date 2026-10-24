#pragma once

#include "domain/result.h"

#include <QLocale>
#include <QObject>
#include <QPalette>
#include <QTranslator>

class QMenu;
class QSettings;

namespace vibmory
{
enum class ColorTheme
{
    System,
    Light,
    Dark
};

// Application presentation settings. They never enter project/undo state.
// The composition root supplies persistent or isolated test settings.
class UiPreferences final : public QObject
{
    Q_OBJECT
public:
    explicit UiPreferences(QSettings& settings, QObject* parent = nullptr);
    ~UiPreferences() override;
    [[nodiscard]] Result<void> initialize(const QString& themeOverride = {},
                                          const QString& languageOverride = {});
    [[nodiscard]] Result<void> setTheme(ColorTheme theme);
    [[nodiscard]] Result<void> setLanguage(const QString& language);
    [[nodiscard]] ColorTheme theme() const
    {
        return theme_;
    }
    [[nodiscard]] QString language() const
    {
        return language_;
    }
    [[nodiscard]] QString appliedLanguage() const
    {
        return appliedLanguage_;
    }
    void populateMenu(QMenu& menu);

signals:
    void themeChanged();
    void notice(const QString& message);

private:
    [[nodiscard]] Result<void> persist(const QString& key,
                                       const QString& value);
    void applyTheme();

    QSettings& settings_;
    ColorTheme theme_ = ColorTheme::System;
    QString language_ = "system";
    QString appliedLanguage_;
    QTranslator translator_;
    QTranslator qtTranslator_;
    QLocale originalLocale_;
    QPalette originalPalette_;
    QString originalStyle_;
    bool initialized_ = false;
};
} // namespace vibmory
