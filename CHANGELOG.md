# Changelog

All notable changes to this project are documented in this file.

## [0.3.0] - 2026-08-07

### Added
- WAV Analyzer CLI for offline analysis (`lsw_audio_diagnostics_cli`)
- Streaming WAV Reader with `WavReadBlockResult`
- Deterministic JSON Report generation
- CLI E2E tests and CTest integration
- Windows, Linux, and macOS CI support

### Changed
- Refactored project structure to include CLI and offline analysis components

## 0.2.0 - 2026-08-06

- Added Peak Hold with sample-time-based decay.
- Added channel diagnostic events and stereo diagnostic events.
- Added `resetLevels`, `resetCounters`, `clearDiagnosticFlags`, and `clearEvents`.
- Extended lock-free Snapshot publication with event information.
- Added the optional Windows Dashboard Example and an actual Dashboard screenshot.
- Preserved v0.1 public names and source-level usage; static-library consumers must rebuild.

## 0.1.0 - 2026-08-06

- Initial pure C++17 static-library release.
- Added mono/stereo level, DC, correlation, channel-state, clip, silence, and finite-value diagnostics.
- Added lock-free Snapshot publication, unit tests, example, install/export CMake package, and CI workflows.
