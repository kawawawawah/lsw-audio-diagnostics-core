# LSW Audio Diagnostics Core agent rules

## Scope

- Work only in this repository.
- Use pure C++17 and the compiler-provided standard library.
- Keep the product a static diagnostics library for mono and stereo audio.
- Win32 and GDI are permitted only inside the optional Windows Dashboard Example.
- Do not expose Win32 headers or types through the public core API.

## Forbidden

- External dependencies, SDKs, device APIs, network features, and copied third-party code.
- Existing product code.
- Heap allocation, mutexes, file I/O, logging, or exceptions in `process()`.
- Unverified success reports.
- Audio device I/O, networking, or external assets in the Dashboard.
- Mock, AI-generated, or third-party images in README screenshots; use only an actual Dashboard capture.

## Required verification

- Release clean build with warnings as errors.
- Unit tests and example execution.
- Inspection of changed files, artifacts, prohibited-code search, third-party marker search, and credential-marker search.
- Report unverified platform and hosted-CI work explicitly.
