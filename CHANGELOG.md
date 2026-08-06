# Changelog

All notable changes to this project are documented in this file.

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
