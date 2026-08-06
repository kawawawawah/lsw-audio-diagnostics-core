# Architecture

`Analyzer<float>` and `Analyzer<double>` are explicitly instantiated by the static library. The public API exposes only configuration, input views, metrics, flags, and snapshots; measurement trackers remain implementation details.

## Processing path

1. `prepare()` validates the fixed configuration and calculates smoothing coefficients.
2. `process()` sanitizes each supplied sample, updates fixed-capacity per-channel trackers, and updates the stereo tracker when two channels are configured.
3. The writer publishes one `Snapshot` after each processed block.
4. `getSnapshot()` copies an internally consistent snapshot for a UI or monitoring thread.

The analyzer supports one or two configured channels only. It has no dynamic channel container, no internal audio buffer, and no ownership of host memory.

## Snapshot sharing

Snapshot publication uses a single-writer sequence guard plus individual 32-bit and 64-bit atomic fields. The writer marks the sequence odd, stores all fields, then marks it even. Sequence and field operations use sequentially consistent ordering; a reader retries if its sequence changed or is odd.

This avoids a mutex and avoids `std::atomic<Snapshot>` or `std::atomic<double>`. The implementation statically requires lock-free 32-bit and 64-bit integer atomics. The intended deployment targets are the documented 64-bit MSVC, GCC, Clang, and AppleClang environments.

## Failure handling

`prepare()` returns `PrepareResult` rather than throwing. `process()` ignores pre-prepare calls, substitutes zero for missing channel data, and records null input, channel-count mismatch, and oversized-block conditions in sticky diagnostic flags. Invalid floating-point samples are also substituted with zero after their counters are updated.
