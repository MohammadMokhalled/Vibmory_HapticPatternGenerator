# Device profiles

Use **Edit > Save device profile** to save the current grid shape, physical channel permutation, gains, and preferred output device in a `.vibprofile` file. Profiles contain no frame stimuli or playback timing. They can be shared independently of projects; output device identities are machine-specific.

Use **Edit > Load device profile** to review a profile. Its grid must match the current project exactly, including row/column orientation. Expand the review details to inspect each physical channel, logical cell, and gain. Applying changes the project's routing through the normal undoable command. Saving the project makes that routing reproducible even without the profile file.

The current output device is retained by default. If the profile's preferred output exists, a checkbox offers to select it explicitly. A missing device never falls back to a different output automatically. Device selection is a machine preference, separate from document undo/redo. Playback still checks exact format support at startup.

Profiles are disabled during preview and WAV export. Invalid input, mismatched dimensions, or failed device selection leaves the working document unchanged. Saving uses atomic replacement with direct-write fallback disabled; validation failure preserves the previous file. Filenames receive their default suffix inside the save dialog before overwrite confirmation.

## Format

Version 1 is a UTF-8 JSON object, limited to 64 KiB:

```json
{
  "format": "vibmory-device-profile",
  "version": 1,
  "name": "Stereo calibration",
  "grid": { "rows": 1, "columns": 2 },
  "outputDeviceId": "",
  "channels": [
    { "cell": 0, "gain": 1 },
    { "cell": 1, "gain": 0.75 }
  ]
}
```

Channel array order is physical output order. `cell` is a zero-based row-major logical cell index. Indices must form a complete permutation and gains must be finite numbers from 0 to 1. Grid dimensions use the same limits as projects. Names contain 1-128 UTF-8 bytes with no control characters; the optional output identity is at most 1024 bytes. Unknown versions, extra/missing fields, incorrect types, fractional indices/dimensions, invalid permutations, and out-of-range gains are rejected.

Per-channel gains are calibration factors, not measurements. No hardware calibration is inferred from a profile. The audio backend port already isolates the Qt implementation; another physical controller requires a documented protocol and device tests before adding a backend. The project does not invent a protocol or a plugin system for the current single implementation.
