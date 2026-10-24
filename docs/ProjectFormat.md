# Project format version 2

Files are UTF-8/ASCII-compatible text. New saves use `.vibmory`. Whitespace separates tokens; numeric values use the C locale. Unknown versions, invalid numbers, excess/truncated content, invalid routing, and resource limits are rejected before publication.

```text
VIBMORY 2
GRID 1 2 1
NEXT_FRAME_ID 2
PLAYBACK 10 5000 48000 continuous
ROUTING 2
0 1
1 0.5
FRAMES
FRAME 1
100 2000
200 3000
END
```

- GRID: positive row count, column count, frame count.
- NEXT_FRAME_ID: the next identity to allocate, as an unsigned 64-bit integer. It must be greater than every stored frame identity. Saving this counter avoids reusing the identity of a deleted frame after reopening the file.
- PLAYBACK: integer frames/second, total duration in milliseconds, integer sample rate Hz, and `continuous` or `legacy` phase.
- ROUTING: physical channel count (must equal cell count), followed by one logical row-major cell index and gain per physical channel. Indices form a complete permutation; gains are finite 0-1.
- FRAMES: each frame starts with `FRAME` and its positive, document-scoped identity, followed by one frequency/amplitude pair per cell, row-major. Identities are unique and travel with frames during reordering. Duplicating a frame allocates a new identity. Sequence order is independent of identity. Newline placement is for readability; token count is strict.
- END: mandatory terminator; no extra non-whitespace content.

Version 1 remains readable. It has no `NEXT_FRAME_ID` or per-frame `FRAME` marker. Import assigns sequential identities starting at 1; subsequent saves write version 2. Version 2 cannot be read by version 1-only applications. No existing project is rewritten until the user saves it. Frame IDs and the allocation counter are part of the undoable document state; stale drag operations are additionally guarded by the monotonic session revision.

Amplitude is a nonnegative signed-PCM peak, capped at 32767. Frequency is 0-22000 Hz and strictly below half the configured sample rate. Frequency zero generates silence. Output is signed 16-bit little-endian interleaved PCM. Duration is floored to a whole sample frame; sequence position is `floor(sampleIndex * fps / sampleRate) % frameCount` using bounded 64-bit arithmetic.

Continuous phase advances an oscillator accumulator per physical channel. Legacy phase uses frequency times absolute sample time; it can be discontinuous when a frame changes frequency. Channel order and gains are explicit and preserved on round-trip. Waveform transitions are not silently smoothed.

Continuous mode rounds samples to the nearest integer. Legacy mode truncates toward zero, matching the old renderer's conversion for valid amplitude values. Both modes bound the final signed PCM peak to 32767.

Mono/stereo WAV export uses ordinary PCM RIFF. More than two channels use WAVEFORMATEXTENSIBLE with the PCM GUID and zero channel mask (unassigned channel positions). RIFF chunk sizes exclude their own 8-byte header. Raw preview does not include a container header. Output budgets keep files below classic RIFF's limit.

## Legacy CSV import

The original writer's header was `frameCount, rows, columns`; another interpretation used `frameCount, columns, rows`. The importer requires an orientation choice. Each frame then contains exactly `rows` lines, each with `columns` frequency/amplitude pairs. One trailing comma per row is accepted. LF/CRLF, ordinary surrounding spaces, and a missing final newline are accepted. Empty interior fields, quoted CSV, nonnumeric tokens, negatives, overflow, extra/missing rows, and out-of-range values are rejected with location context.

Legacy imports set absolute phase and an explicit column-major channel map, matching the original audio iteration. They receive documented default timing (10 frames/s, 5 seconds, 48 kHz) because those settings were not saved in old files. Save as `.vibmory` to preserve the chosen interpretation and later settings. The importer cannot infer a researcher's intended wiring from an ambiguous rectangular header.
