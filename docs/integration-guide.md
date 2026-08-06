# Integration guide

Create one Analyzer per audio stream. In your non-real-time setup stage, fill `AnalyzerConfig` with the current sample rate, maximum host block size, and mono/stereo channel count, then call `prepare()` and check for `PrepareResult::success`.

In the audio callback, pass host channel pointers directly to `process()`. Do not allocate, reconfigure, reset, or copy the host audio block for this library. Fetch `Snapshot` from a meter, editor, or monitoring thread.

For a plugin, create the Analyzer after construction and call `prepare()` whenever the host changes sample rate, channel arrangement, or maximum block size. For an application, use one Analyzer after the device configuration is stable. Never treat `monoCompatibilityScore` as a delivery-spec compliance result; use it only as a quick diagnostic hint.
