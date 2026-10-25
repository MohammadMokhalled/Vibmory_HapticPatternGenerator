# Contributing to Vibmory

Use the supported toolchain and commands in [README](README.md). Read [Architecture](docs/Architecture.md) and [Project format](docs/ProjectFormat.md) before changing document, playback, or persistence behavior.

## Responsibilities and names

Keep invariants in `domain`, commands and lifecycle policy in `application`, numeric samples/WAV in `audio`, project grammar in `persistence`, Qt file/device adapters in `platform`, and presentation in `ui`. Construct dependencies in `app/main.cpp`. Introduce a replaceable interface where tests or a real boundary need one.

Use `PascalCase` for types, `lowerCamelCase` for functions/signals/local values, a trailing underscore for private members, and `snake_case` for source/header filenames. Name values by meaning and units: `row`, `column`, `frequencyHz`, `durationMs`, `sampleRate`, and `frameRate`. A frame is a complete grid of cell stimuli; an audio sample frame contains one sample per physical channel. Distinguish those meanings in APIs and documentation. Rows are vertical, columns are horizontal; logical cells are row-major and physical routing is explicit.

Domain APIs return `Result` for recoverable validation/operation failures. Checked internal queries such as `frame(index)` require a valid index and throw `std::out_of_range` for programmer misuse; `Result::value()` requires a successful result. Do not catch these contract violations and treat them as successful operations. File/backend/worker boundaries translate operational failures into contextual errors. Painting, view refresh, and selection must not issue document edits.

The GUI thread owns mutable sessions and Qt widgets/devices. Workers receive document copies and return results through queued delivery. Do not capture a widget or mutable session in worker code. Keep cancellation and shutdown ownership explicit and test stale completion, restart, and destruction behavior when changing a job.

## Checks and review

Install the pinned formatter with `python -m pip install clang-format==21.1.0`. Format changed owned C++ with `clang-format -i path/to/file.cpp`, then run:

```powershell
python tools/check_quality.py
python tools/check_translations.py
cmake --build --preset core
ctest --preset core
```

Use the desktop preset and Qt UI tests for UI/adapters. CI also checks Linux sanitizers, focused Clang analysis, Doxygen, desktop builds with tests disabled, real packaged file workflows, installer lifecycle, and runtime/source provenance. See README for a Visual Studio generator alternative and static-analysis setup. Core tests need no Qt, sound device, or downloaded test framework.

Add focused behavioral regressions for changed invariants and failure paths. Use asymmetric rectangular grids, fake storage/audio/time where appropriate, and RAII temporary files. Sample or format changes need independently expected values; avoid tests that only repeat the implementation. Keep hardware routing/latency measurements separate from fake-backend results.

Keep semantic changes reviewable. Describe the observed problem, resulting behavior, validation, and any file/channel/phase compatibility effect. Keep broad mechanical formatting or renaming separate from behavior changes. Update architecture, format, translation catalog, and relevant acceptance evidence with the implementation.

After UI text changes, run the documented `update_translations` target and Qt extraction check. Translate new catalog entries and preserve placeholders, accelerators, and filters. Detailed diagnostic context currently remains English as described in [Appearance and language](docs/AppearanceAndLanguage.md).

Dependency changes require an explicit version, integrity pin, license/notice review, and matching source provenance. Unknown runtime components deliberately fail packaging. Follow [Distribution](docs/Distribution.md) and [Releases](docs/Releases.md) when changing deployment. Never put signing credentials in source control. Generated output belongs under ignored build/stage/artifact directories; shared presets, authored resources, and intentional fixtures remain tracked.
