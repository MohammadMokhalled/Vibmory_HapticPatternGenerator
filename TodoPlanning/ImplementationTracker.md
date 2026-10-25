# Refactor implementation tracker

All 69 software items from the audit, including the optional extensions authorized in the follow-up, are implemented and verified. [Completion audit](CompletionAudit.md) maps each requirement to its owner and evidence and records the supported-platform, hardware-test, and signing boundaries. The original audit remains a historical source review.

## Delivery stages

1. **Core:** validated standard C++ domain, transactional commands, bounded grouped undo/redo, deterministic PCM/WAV, strict versioned codec and explicit legacy compatibility. Nine local/Linux suites pass.
2. **Desktop:** one owned Qt 6.8 editor, numeric grid/inspector projections, stable timeline model, atomic files and isolated cancellable workers. Twelve Windows suites and the deployed native file workflow pass.
3. **Delivery:** pinned CI, sanitizer/analysis/docs gates, test-disabled desktop build, app-local runtime, verified portable ZIP, per-user installer, optional signing, companion dependency sources and actual draft releases.
4. **Extended scope:** routing/calibration profiles, recovery with terminated-process tests, theme/icons/German UI, property/mutation tests, streaming and benchmark tooling are complete. See the completion audit for the requirement-by-requirement review.

## Decisions

- Qt 6.8 or newer; CI pins 6.8.3 on Windows x64. Qt 5 support is removed.
- Core tests use a small dependency-free CTest runner instead of repairing a missing GoogleTest checkout. GUI tests use Qt Test. No test downloads are needed for the core.
- Domain/application/codec/PCM modules use standard C++20 so local testing does not need Qt.
- Canonical projects use a bounded versioned `.vibmory` text format. Legacy CSV import requires an explicit orientation and retains legacy physical channel order and phase mode.
- Playback streams a fixed snapshot. Editing is locked during playback/export. Preview never depends on a temporary filename.
- Frequency must remain below Nyquist, amplitude is signed-PCM peak 0-32767, maximum 256 channels/1024 frames/262144 stored cells/512 MiB rendered audio.
- Project-owned code uses MIT under the maintainer's delegated decision authority. Existing third-party terms remain independent. Pinned archives, notices, runtime inventory and companion-source delivery passed distribution and actual tagged-release verification.

## Status meaning

Verified (software) means the implemented behavior has source/build/test evidence. Physical actuator/driver/latency validation belongs to the user's later device test. Signing support is configured; credentialed signing requires an actual certificate, and current builds explicitly report that they are unsigned. All items below are closed within those documented boundaries.

## Item status

| ID | Priority | Requirement | Status |
| --- | --- | --- | --- |
| R01 | P0 | — Eliminate project-window initialization undefined behavior | Verified (software) |
| R02 | P0 | — Initialize a loaded document's current frame | Verified (software) |
| R03 | P0 | — Establish one grid-coordinate convention end to end | Verified (software) |
| R04 | P0 | — Enforce document, frame, and numeric invariants at the boundary | Verified (software) |
| R05 | P0 | — Make frame edits transactional across the model and selector | Verified (software) |
| R06 | P0 | — Resolve unsupported frame-tab dragging | Verified (software) |
| R07 | P0 | — Repair the generated-audio/Play contract | Verified (software) |
| R08 | P0 | — Make project saves atomic and failure-aware | Verified (software) |
| R09 | P0 | — Stop reporting successful audio work after failure | Verified (software) |
| R10 | P0 | — Separate raw PCM preview data from valid WAV export | Verified (software) |
| R11 | P0 | — Make supported Qt versions and dependencies truthful | Verified (software) |
| R12 | P1 | — Introduce enforceable module boundaries | Verified (software) |
| R13 | P1 | — Make frames and documents data models | Verified (software) |
| R14 | P1 | — Make ownership explicit and construction exception-safe | Verified (software) |
| R15 | P1 | — Give polymorphic backend contracts safe destruction | Verified (software) |
| R16 | P1 | — Define project close, replacement, and application shutdown | Verified (software) |
| R17 | P1 | — Choose one recoverable error model | Verified (software) |
| R18 | P1 | — Centralize editor selection and remove sentinel-driven APIs | Verified (software) |
| R19 | P1 | — Replace button text and boolean fragments with playback state | Verified (software) |
| R20 | P1 | — Make painting a side-effect-free projection | Verified (software) |
| R21 | P1 | — Use one elapsed-time model for audiovisual playback | Verified (software) |
| R22 | P1 | — Render immutable snapshots and track preparation validity | Verified (software) |
| R23 | P1 | — Prevent view refreshes from editing the document | Verified (software) |
| R24 | P1 | — Remove initialization-event counting and signal-order workarounds | Verified (software) |
| R25 | P1 | — Adopt and document a real thread-safety policy | Verified (software) |
| R26 | P1 | — Separate deterministic synthesis from devices and files | Verified (software) |
| R27 | P1 | — Correct and bound audio timing/arithmetic | Verified (software) |
| R28 | P1 | — Make device capabilities and physical channel mapping explicit | Verified (software) |
| R29 | P1 | — Specify waveform transitions instead of inheriting accidental phase behavior | Verified (software) |
| R30 | P1 | — Give temporary audio a session-scoped lifetime | Verified (software) |
| R31 | P1 | — Define a versioned project format and a strict legacy reader | Verified (software) |
| R32 | P1 | — Persist the complete reproducible project configuration | Verified (software) |
| R33 | P1 | — Track unsaved changes and document identity | Verified (software) |
| R34 | P1 | — Bound rendering work and keep the GUI responsive | Verified (software) |
| R35 | P1 | — Introduce useful error context and deliberate logging | Verified (software) |
| R36 | P1 | — Replace the all-purpose object collection with target-scoped build rules | Verified (software) |
| R37 | P1 | — Make tests optional to build and dependencies reproducible | Verified (software) |
| R38 | P1 | — Standardize build presets and compiler/runtime settings | Verified (software) |
| R39 | P1 | — Add meaningful domain, persistence, and sample regressions | Verified (software) |
| R40 | P1 | — Test application workflows with fake playback, storage, and time | Verified (software) |
| R41 | P1 | — Add a small high-value Qt UI integration suite | Verified (software) |
| R42 | P1 | — Add repeatable CI and diagnostic builds | Verified (software) |
| R43 | P2 | — Replace idle repaint polling with explicit updates | Verified (software) |
| R44 | P2 | — Finish grid geometry and accessibility behavior | Verified (software) |
| R45 | P2 | — Simplify startup and new-project navigation | Verified (software) |
| R46 | P2 | — Make Designer forms and action wiring deliberate | Verified (software) |
| R47 | P2 | — Adopt a precise naming and vocabulary guide | Verified (software) |
| R48 | P2 | — Apply and enforce one formatter configuration | Verified (software) |
| R49 | P2 | — Clean include dependencies, constants, and ordinary C++ contracts | Verified (software) |
| R50 | P2 | — Consolidate cell storage if it improves invariants | Verified (software) |
| R51 | P2 | — Remove dead state, APIs, and boilerplate | Verified (software) |
| R52 | P2 | — Consolidate repeated UI construction and view-state updates | Verified (software) |
| R53 | P2 | — Make existing tests small, isolated, and informative | Verified (software) |
| R54 | P2 | — Add lightweight style and dependency guardrails | Verified (software) |
| R55 | P2 | — Replace README onboarding TODO with verified instructions | Verified (software) |
| R56 | P2 | — Keep architecture, decisions, and diagrams current | Verified (software) |
| R57 | P2 | — Fix Doxygen input and output configuration | Verified (software) |
| R58 | P2 | — Verify an installable, runnable application artifact | Verified (software) |
| R59 | P2 | — Make repository hygiene intentional | Verified (software) |
| R60 | P2 | — Align project metadata and preserve third-party notices | Verified (software) |
| R61 | P3 | — Undo/redo through editor commands | Verified (software) |
| R62 | P3 | — Advanced timeline and stable frame identities | Verified (software) |
| R63 | P3 | — Real-time streaming and low-latency synthesis | Verified (software) |
| R64 | P3 | — Extended fuzzing, property testing, and benchmark infrastructure | Verified (software) |
| R65 | P3 | — Remove all Qt dependencies from the domain | Verified (software) |
| R66 | P3 | — Introduce a custom visual theme, icons, and richer localization | Verified (software) |
| R67 | P3 | — Add actuator backends, calibration, and device profiles | Verified (software) |
| R68 | P3 | — Autosave and crash recovery | Verified (software) |
| R69 | P3 | — Broaden release automation and platform coverage | Verified (software) |

## Completion checkpoint

- Implementation source `729f77c` passed branch run 40222527129 and tagged run 40222589069. Both build jobs and real draft assembly passed; draft `v2.0.0-rc.2` has portable/setup/source ZIPs and four checksum assets.
- Downloaded reports: nine Linux suites, twelve Windows suites; 33 UI, nine recovery, six preferences and four deployed native cases including fixtures; zero failures/errors/skips. Atomic replacement failure preserves previous project/WAV bytes, both rectangular file workflows pass, and move controls reflect sequence boundaries.
- The release assembler checked source/tag/version equality, all runtime/source hashes and provenance, seven installer lifecycle results, and unsigned-report consistency before creating its draft. Actual signing and physical output remain the explicit external checks above.
- Local source `15c7935` downloads were independently checked: portable SHA-256 `e0c35f7e7566848ca7521ae25c7b0e130eac6bfda4e9a9a2bea6c4355184c51d`, setup `26d24559c2e08d5136218d0088cb4e36be3f5dc301074f62658ac65e538fe28c`; 71 runtime entries, seven archives; native launch without Qt exited 0. The later code change only corrects timeline move-action enabled state and has its own passing regression.

## Historical verification log

The following checkpoints record progress at their stated commits, including issues that were subsequently closed. Use the completion audit/checkpoint above for current status.

- Local: `cmake -S . -B out/core -DVIBMORY_BUILD_GUI=OFF -DVIBMORY_WARNINGS_AS_ERRORS=ON`; MSVC 19.50.
- Local: `cmake --build out/core --config Debug --parallel 4`; passed.
- Local: `ctest --test-dir out/core -C Debug --output-on-failure`; 5/5 suites passed.
- GitHub run 39806088336 / commit 2e224cc: Windows Qt build, 8 CTest suites, expanded Qt modal tests, install, native package smoke test, screenshot and ZIP upload passed. Linux sanitizer/quality/tests passed; Doxygen stopped on missing planning-document inputs.
- Downloaded artifact 12437962351 (Vibmory-windows-x64); verified SHA-256 4c4447846d6afedb3bb80a7ef0215a1fa2e847a8b92bf8be347863972300cfde and 65 archive entries.
- Actual downloaded executable launched locally with Qt removed from PATH and exited 0 using --smoke-test. Reviewed editor.png; controls and grid fit at the default size. Local files: artifacts/39806088336/ (ignored build evidence).
- Local current core: 7/7 CTest suites pass, including 500 generated project round trips, 10000 parser mutations and independent stereo/multichannel WAV decoding. First/middle/last deletion regressions passed both core and Qt UI suites.
- Test-disabled Release configuration built successfully without Qt or test dependencies.
- Informational local Debug renderer benchmark: 16 channels x 48 kHz x 10 seconds rendered in approximately 0.25 wall seconds. This is not a latency or physical-output measurement.

## Historical implementation stages

1. Keep the final branch head green after each change and refresh the checked artifact/evidence before the completion audit.
2. Inspector, asynchronous export job, save-path dialog policy, document workflow, timeline model, profile actions, and recovery controller have been extracted. Preserve these responsibilities while completing presentation and release work.
3. Categorized diagnostics and focused Clang analysis are implemented and passed with LLVM 21.1.0. Finish the deployment license/attribution inventory and remaining item acceptance gaps. Preserve existing project rights while documenting distribution terms.
4. Finish tagged release/installer/signing configuration where credentials are available. Existing presentation, timeline, profiles, recovery, undo, bounded streaming, property tests/benchmark and standard C++ core provide the other extended items.
5. Audit every requirement against current source and test/artifact evidence. Only then mark the goal complete. Physical routing and latency checks remain for the user's intended device, as requested.

## Verified foundation checkpoint

- Full workflow 39813346547 / source d4c9b18 passed: unchanged focused analysis checks under pinned LLVM 21.1.0, compiler warnings as errors, sanitizers, 7 Linux CTest suites, 8 Windows CTest suites, Doxygen warnings as errors, native Windows package launch, screenshot, ZIP and checksum.
- Earlier fully green runs also verified the inspector extraction (39808181239), export worker lifecycle (39809445979), document workflow (39810165862), and output-device preservation/shutdown ownership (39810914332).
- The distribution with app-local C++ runtime was downloaded from 39807471809 and verified: SHA-256 1a25e084cd93563e87fa94bd4b45970258bf191f2e75e60f313c1d78aa306466; 75 entries, including msvcp140.dll, vcruntime140.dll and vcruntime140_1.dll.
- No analysis checks were suppressed. LLVM 18's variant error-copy diagnostic was not reproduced under the pinned 21.1.0 analyzer; Error construction additionally requires an explicit code/message.
- Save/export filenames now receive their default suffix inside QFileDialog before overwrite confirmation; the application does not rewrite the chosen path afterward.
- At this foundation checkpoint, R69 delivery and the final audit remained open. Both are now closed by the completion checkpoint above. Physical channel/latency testing belongs to the user's later device check.

## Timeline and profile checkpoints

- Run 40126245946 / c08e39c passed both jobs: durable frame IDs, version 2 save/load with version 1 migration, identical-frame reorder/undo regression, exact saved playback order, Qt model contract checks, stale/foreign drag rejection, cached thumbnail refresh, and packaged Windows launch. Windows artifact: 12573746813.
- Device profiles passed run 40126595002 / cb5929b, including both full jobs, validated application behavior, bounded JSON corruption cases, Unicode round trip, atomic-save preservation, Doxygen, and packaged Windows launch. Windows artifact: 12573367624.
- Recovery passed run 40127308000 / b46928d: both jobs, nine Windows suites, seven Linux suites, static analysis, Doxygen, and packaged launch. Tests exercise a terminated child leaving a stale process lock, active-instance exclusion, retention, immutable background snapshots, cancellation/restart, worker exceptions, project replacement, startup restore/discard/keep, and accepted window-close cleanup.
- Downloaded profile artifact 12573367624 from 40126595002; SHA-256 9b8d021807d1cc7ae13f45a067ec6efed7d0dbd8b0e87b65e8298ac25b813b7a; 75 ZIP entries with app-local runtime/plugins. It launched locally with Qt removed from PATH (exit 0). Reviewed its editor screenshot and thumbnail timeline layout.
- Run 40127642913 / 775687f passed all jobs. Downloaded and parsed successful JUnit reports: seven Linux suites, nine Windows suites, 26 UI cases and seven recovery cases plus fixtures, zero failures/errors/skips. Downloaded Windows artifact 12573114829; SHA-256 676023566be6de4c2cbeeb7b84b38d223d0a30c67011a938efb0c0e06ed02391; 75 entries. The extracted executable launched locally without Qt on PATH and exited 0.
- Presentation passed run 40170327591 / 19c5d5f: all jobs, ten Windows suites, Qt extraction, contrast/preferences tests, and deployed English/light and German/dark previews. Visual review found and corrected dark placeholder/media-icon/silent-cell colors; widget palette-event tests verify thumbnail/icon refresh without changing the document. The earlier extraction guard correctly caught a missing project include path, which was fixed rather than bypassed.
- Distribution passed run 40171094632 / 247f94b, including all jobs, eight Linux/eleven Windows suites, package/source validation and native previews. It includes MIT licensing, CMake-generated name/version/revision metadata, offline documentation, pinned Qt/FFmpeg/Mesa/LLVM sources, upstream/vendor notices, FFmpeg build checks, and a binary-component/hash/version inventory. Runtime artifact: 12590191703; companion source artifact: 12590216639.
- A pinned NSIS 3.13 per-user installer compiles locally with warnings as errors. Its uninstall manifest removes only packaged files, keeps nonempty user directories, and uses separate installer registration keys. Optional Authenticode helpers are configured; the repository currently has no signing secrets, so test builds are unsigned. CI install/upgrade/Unicode-path/busy-file/uninstall checks and draft-release delivery remain pending.

- Local installer lifecycle QA passed all seven acceptance results after correcting NSIS handling of a not-yet-created install directory. Installation, upgrade, native launch without Qt, both busy-file guards, user-project preservation, and uninstall passed in a Unicode/spaced path. Draft-release gates have four passing regression tests; full CI and a real draft assembly remain pending. See [Releases](../docs/Releases.md).

- Run 40220428752 / 6c2b908 passed both jobs, including nine Linux/twelve Windows suites, installer lifecycle QA, distribution validation, and deployed previews. The draft job correctly skipped an ordinary branch push. A native deployed-runtime save/open/export probe, explicit test-disabled desktop build, and independent mono WAV decoding were added during the final acceptance audit and await CI. Real draft assembly remains pending.

- Final waveform review found that continuous phase could hold a nonzero sample during a zero-frequency frame. A regression reproduced the failure before the renderer was corrected. Both phase modes now verify exact silence for zero-frequency and zero-amplitude frames, followed by resumed output; this closes a gap in R04/R29's documented zero-frequency policy.

- Run 40221209920 / d66a150 passed all twelve Windows suites (including both distinct rectangular file workflows), all nine Linux suites, sanitizer/static analysis, the test-disabled desktop build, distribution, installer lifecycle and previews. The native probe step then found a wrong test-executable path; Qt's project setup places it at `out/desktop/vibmory_ui_tests.exe`. The path was corrected. Timing limits now have a shared domain definition consumed by the UI; exact valid/invalid bounds and the final-frame-count budget have local regressions. Final-source CI and draft integration remain pending.
