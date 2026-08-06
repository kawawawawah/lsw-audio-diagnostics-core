# Contributing

Thank you for contributing to LSW Audio Diagnostics Core.

## Development setup

Use CMake 3.22 or newer and a C++17 compiler. Configure with tests, examples, and warnings-as-errors enabled, then run a Release clean build and CTest before opening a pull request.

## Requirements

- Use four spaces, UTF-8, LF line endings, `#pragma once`, `nullptr`, scoped enums, `[[nodiscard]]`, and `noexcept` where appropriate.
- Add focused tests for every behavior change.
- Do not add external dependencies, SDKs, assets, telemetry, or network features.
- Do not copy third-party or existing product code.
- Keep `process()` allocation-free, lock-free, file-I/O-free, log-free, and non-throwing.
- Retain SPDX headers in C++ source and header files.

## Pull requests

Describe the motivation, realtime-safety impact, API compatibility impact, tests, and licensing status. Keep commits intentional and avoid generated build files.
