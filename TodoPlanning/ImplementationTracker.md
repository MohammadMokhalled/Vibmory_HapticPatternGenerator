# Refactor implementation tracker

This tracks the full 69-item audit, including optional items requested in the follow-up. An implemented source change is not complete until the relevant tests/build/artifact evidence exists. The original audit remains a historical source review.

## Delivery stages

1. **Core contracts and regression tests:** validated standard C++ domain, transactional editor session, bounded undo/redo, deterministic PCM/WAV rendering, strict versioned project codec, explicit legacy CSV orientation/routing. Local MSVC Debug core suites currently pass with warnings as errors.
2. **Qt 6 desktop integration:** one owned editor window, pure grid projection, signal-blocked inspector, state-based streamed playback, atomic file store, cancellable asynchronous export, explicit routing/gains/device selection. Source implemented; remote compilation/UI verification pending.
3. **Build/test/package delivery:** pinned GitHub Actions, Linux sanitizer core tests, Windows Qt 6.8.3 build/UI tests/deployed ZIP/smoke test. Must verify the actual pushed run and download/inspect its artifact.
4. **Polish and extended scope:** finish all remaining documentation, diagnostics, lifecycle/UI regressions, stable timeline identities, profiles, localization/theme, recovery, fuzz/benchmark tooling, and release automation. Items remain open until evidenced.

## Decisions

- Qt 6.8 or newer; CI pins 6.8.3 on Windows x64. Qt 5 support is removed.
- Core tests use a small dependency-free CTest runner instead of repairing a missing GoogleTest checkout. GUI tests use Qt Test. No test downloads are needed for the core.
- Domain/application/codec/PCM modules use standard C++20 so local testing does not need Qt.
- Canonical projects use a bounded versioned `.vibmory` text format. Legacy CSV import requires an explicit orientation and retains legacy physical channel order and phase mode.
- Playback streams a fixed snapshot. Editing is locked during playback/export. Preview never depends on a temporary filename.
- Frequency must remain below Nyquist, amplitude is signed-PCM peak 0-32767, maximum 256 channels/1024 frames/262144 stored cells/512 MiB rendered audio.
- Existing third-party notices are preserved. Distribution/license metadata and bundled Qt notices still require completion.

## Item status

| ID | Priority | Requirement | Status |
| --- | --- | --- | --- |
| R01 | P0 | — Eliminate project-window initialization undefined behavior | In progress |
| R02 | P0 | — Initialize a loaded document's current frame | In progress |
| R03 | P0 | — Establish one grid-coordinate convention end to end | In progress |
| R04 | P0 | — Enforce document, frame, and numeric invariants at the boundary | In progress |
| R05 | P0 | — Make frame edits transactional across the model and selector | In progress |
| R06 | P0 | — Resolve unsupported frame-tab dragging | In progress |
| R07 | P0 | — Repair the generated-audio/Play contract | In progress |
| R08 | P0 | — Make project saves atomic and failure-aware | In progress |
| R09 | P0 | — Stop reporting successful audio work after failure | In progress |
| R10 | P0 | — Separate raw PCM preview data from valid WAV export | In progress |
| R11 | P0 | — Make supported Qt versions and dependencies truthful | In progress |
| R12 | P1 | — Introduce enforceable module boundaries | In progress |
| R13 | P1 | — Make frames and documents data models | In progress |
| R14 | P1 | — Make ownership explicit and construction exception-safe | In progress |
| R15 | P1 | — Give polymorphic backend contracts safe destruction | In progress |
| R16 | P1 | — Define project close, replacement, and application shutdown | In progress |
| R17 | P1 | — Choose one recoverable error model | In progress |
| R18 | P1 | — Centralize editor selection and remove sentinel-driven APIs | In progress |
| R19 | P1 | — Replace button text and boolean fragments with playback state | In progress |
| R20 | P1 | — Make painting a side-effect-free projection | In progress |
| R21 | P1 | — Use one elapsed-time model for audiovisual playback | In progress |
| R22 | P1 | — Render immutable snapshots and track preparation validity | In progress |
| R23 | P1 | — Prevent view refreshes from editing the document | In progress |
| R24 | P1 | — Remove initialization-event counting and signal-order workarounds | In progress |
| R25 | P1 | — Adopt and document a real thread-safety policy | In progress |
| R26 | P1 | — Separate deterministic synthesis from devices and files | In progress |
| R27 | P1 | — Correct and bound audio timing/arithmetic | In progress |
| R28 | P1 | — Make device capabilities and physical channel mapping explicit | In progress |
| R29 | P1 | — Specify waveform transitions instead of inheriting accidental phase behavior | In progress |
| R30 | P1 | — Give temporary audio a session-scoped lifetime | In progress |
| R31 | P1 | — Define a versioned project format and a strict legacy reader | In progress |
| R32 | P1 | — Persist the complete reproducible project configuration | In progress |
| R33 | P1 | — Track unsaved changes and document identity | In progress |
| R34 | P1 | — Bound rendering work and keep the GUI responsive | In progress |
| R35 | P1 | — Introduce useful error context and deliberate logging | In progress |
| R36 | P1 | — Replace the all-purpose object collection with target-scoped build rules | In progress |
| R37 | P1 | — Make tests optional to build and dependencies reproducible | In progress |
| R38 | P1 | — Standardize build presets and compiler/runtime settings | In progress |
| R39 | P1 | — Add meaningful domain, persistence, and sample regressions | In progress |
| R40 | P1 | — Test application workflows with fake playback, storage, and time | In progress |
| R41 | P1 | — Add a small high-value Qt UI integration suite | In progress |
| R42 | P1 | — Add repeatable CI and diagnostic builds | In progress |
| R43 | P2 | — Replace idle repaint polling with explicit updates | In progress |
| R44 | P2 | — Finish grid geometry and accessibility behavior | In progress |
| R45 | P2 | — Simplify startup and new-project navigation | In progress |
| R46 | P2 | — Make Designer forms and action wiring deliberate | In progress |
| R47 | P2 | — Adopt a precise naming and vocabulary guide | In progress |
| R48 | P2 | — Apply and enforce one formatter configuration | In progress |
| R49 | P2 | — Clean include dependencies, constants, and ordinary C++ contracts | In progress |
| R50 | P2 | — Consolidate cell storage if it improves invariants | In progress |
| R51 | P2 | — Remove dead state, APIs, and boilerplate | In progress |
| R52 | P2 | — Consolidate repeated UI construction and view-state updates | In progress |
| R53 | P2 | — Make existing tests small, isolated, and informative | In progress |
| R54 | P2 | — Add lightweight style and dependency guardrails | In progress |
| R55 | P2 | — Replace README onboarding TODO with verified instructions | In progress |
| R56 | P2 | — Keep architecture, decisions, and diagrams current | In progress |
| R57 | P2 | — Fix Doxygen input and output configuration | In progress |
| R58 | P2 | — Verify an installable, runnable application artifact | In progress |
| R59 | P2 | — Make repository hygiene intentional | In progress |
| R60 | P2 | — Align project metadata and preserve third-party notices | In progress |
| R61 | P3 | — Undo/redo through editor commands | In progress |
| R62 | P3 | — Advanced timeline and stable frame identities | Pending |
| R63 | P3 | — Real-time streaming and low-latency synthesis | In progress |
| R64 | P3 | — Extended fuzzing, property testing, and benchmark infrastructure | Pending |
| R65 | P3 | — Remove all Qt dependencies from the domain | In progress |
| R66 | P3 | — Introduce a custom visual theme, icons, and richer localization | Pending |
| R67 | P3 | — Add actuator backends, calibration, and device profiles | In progress |
| R68 | P3 | — Autosave and crash recovery | Pending |
| R69 | P3 | — Broaden release automation and platform coverage | In progress |

## Verification log

- Local: `cmake -S . -B out/core -DVIBMORY_BUILD_GUI=OFF -DVIBMORY_WARNINGS_AS_ERRORS=ON`; MSVC 19.50.
- Local: `cmake --build out/core --config Debug --parallel 4`; passed.
- Local: `ctest --test-dir out/core -C Debug --output-on-failure`; 5/5 suites passed.
- GitHub: access verified; workflow execution and artifact verification pending first push.
