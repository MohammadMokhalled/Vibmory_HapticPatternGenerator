# Appearance and language

**View > Theme** offers Follow system, Light, and Dark. Themes use Qt's Fusion widget style and a shared palette, with blue accents, readable text, and standard Qt action icons. Follow system selects the light/dark palette from the platform color scheme and follows later scheme changes. Theme changes apply immediately without modifying the project, selection, undo history, playback snapshot, or export.

**View > Language (next launch)** offers System language, English, and Deutsch. German system locales select German; other system languages currently use English UI text. Choosing a language saves the preference and reports that it applies after restarting. The open document is not rebuilt or replaced. Menus, controls, accessibility labels, tooltips, recovery/profile dialogs, and Qt's standard dialogs have German translations. Detailed parser, device, filesystem, and other diagnostic messages retain their English technical context.

Preferences use Qt's per-user application settings and are independent of `.vibmory` projects. Invalid saved values fall back to the system setting. Preference-write errors are reported in the status bar. Language-specific decimal separators affect controls; project/profile encoders continue using their defined locale-independent formats.

One-launch overrides are available:

```powershell
Vibmory.exe --theme dark --language de
```

Valid themes are `system`, `light`, and `dark`; valid languages are `system`, `en`, and `de`. Overrides do not write settings. Smoke-test and screenshot modes use isolated temporary settings and disable recovery. CI captures both English/light and German/dark packaged previews with Qt removed from PATH.

## Maintaining translations

The reviewed German catalog is `translations/vibmory_de.ts`. CMake's `release_translations` target compiles it, and the UI library embeds the QM resource explicitly so applications and tests use the same catalog. Qt's own dialog translation is supplied by the SDK during development and the adjacent deployed `translations` folder in the portable package.

After changing UI text:

```powershell
cmake --build out/desktop --target update_translations
python tools/check_translations.py --lupdate lupdate
```

Review and translate new/changed TS entries before committing. The checker rejects unfinished messages, missing UI literals, duplicate keys, changed placeholders, lost keyboard accelerators, and altered file filters. Windows CI also compares the catalog with Qt's actual `lupdate` extraction; Linux runs the catalog/source checks without Qt. Preference tests verify language persistence and next-launch behavior, German standard dialog text, locale-independent serialization, and active-text contrast for both palettes. Existing UI tests also check that changing presentation leaves the session unchanged.
