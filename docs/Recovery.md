# Recovery snapshots

The editor captures unsaved document changes every 30 seconds in a background job. It keeps one atomic snapshot per running session, separate from the explicitly saved project. A crash can lose edits made since the last completed snapshot. This feature complements ordinary saves.

On Windows snapshots live below the application's local data directory, normally `%LOCALAPPDATA%/Vibmory/Vibmory/recovery`. The exact base directory comes from Qt's `AppLocalDataLocation`. Files use UUID names with a `.vibrecovery` suffix and an accompanying process lock. A snapshot preserves the complete canonical document, the original absolute project path when available, and its capture time.

Starting the editor without a project argument offers earlier inactive snapshots:

- **Restore** opens the selected document as unsaved work. Save explicitly to update its original project, or use Save as to choose another path. Selection starts at the first frame. The recovered snapshot stays protected by this session's lock until it is saved, discarded, replaced, or closed normally.
- **Discard selected** removes only that recovery snapshot. Corrupt snapshots can be discarded but cannot be restored.
- **Keep for later** leaves the snapshots for a later launch. Opening a project via the command line skips this prompt and preserves earlier snapshots.

Snapshots belonging to another live editor are excluded. Long editing sessions do not expire their locks; Qt reclaims locks belonging to terminated processes. Inactive snapshots are retained for up to 30 days, with at most the ten newest offered sessions. Retention cleanup only touches UUID-named recovery files in the recovery directory and never removes a locked active snapshot. Orphaned locks from a terminated process that never wrote its first snapshot are also reclaimed.

Successful explicit save, undo back to the saved state, document replacement, and normal accepted close cancel/join pending snapshot work and remove that session's snapshot. Canceling a close keeps recovery active. Failed writes are reported in the status bar and log; they do not mark the document saved. Cancellation or I/O failure preserves the previous complete snapshot. Recovery never writes automatically to the stored original project path.

`--no-recovery` disables snapshot writes and startup recovery prompts. `--smoke-test` and `--screenshot` disable recovery so automated checks do not access persistent user recovery state. Tests inject temporary directories and replaceable write operations.

## Storage and concurrency contract

The GUI thread owns the editable session, lease, timer, and future watcher. A worker receives a document copy, path, timestamp, and shared cancellation flag. It creates its own atomic file writer, checks cancellation between bounded chunks and before commit, and never accesses the live session or widgets. Only one write runs at a time. A later edit remains eligible for the next capture even if an older snapshot finishes successfully. A document generation distinguishes New/Open/Restore from ordinary revisions; replacement joins old work before clearing its snapshot.

Recovery format version 1 contains an exact `VIBMORY_RECOVERY 1` line, one compact JSON metadata line (`originalPath` and `capturedAt`), and a canonical `.vibmory` project. Metadata is limited to 8 KiB, the original path to 4096 UTF-8 bytes, and the embedded project to its ordinary 16 MiB input limit. Invalid versions, metadata types, relative stored paths, invalid dates, and invalid embedded projects are rejected before restoration. Recovery files are local application state, not the exchange format for projects.
