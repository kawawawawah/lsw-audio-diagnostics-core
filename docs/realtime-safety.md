# Real-time safety

## `prepare()`

`prepare()` validates the configuration, calculates smoothing coefficients, clears fixed state, and publishes an initial snapshot. It is not for use in the audio callback.

## `process()`

After a successful `prepare()`, `process()` performs bounded loops over caller-owned samples, updates fixed-size level, correlation, and event state for at most two channels, and stores atomic snapshot fields. It performs no heap allocation, deallocation, locking, file I/O, logging, exception propagation, operating-system calls, or blocking work.

## Snapshot readers

`getSnapshot()` is intended for UI or monitoring readers and uses an atomic sequence retry. The audio writer never waits for readers. Do not call `prepare()` or `reset()` concurrently with `process()`, and use one writer thread per Analyzer instance.

## Caller obligations

- Call `prepare()` successfully before normal processing.
- Keep every non-null channel pointer valid for `numberOfSamples`.
- Keep input data read-only while the call is in progress.
- Use only one or two configured channels.
- Treat `maximumBlockSize` mismatch as a host integration error even though the fixed-storage implementation safely analyses the supplied block and reports the flag.
