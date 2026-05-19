# Changelog

This file records user-visible changes before they are released.

Maintenance rules:

- Keep current, unreleased work under `## [Unreleased]`.
- Move accumulated entries into `## [vX.Y.Z] - YYYY-MM-DD` only when creating a release tag.
- Use semantic versioning: major for breaking or architectural changes, minor for new features, patch for fixes and small maintenance.
- Keep entries concise and focused on behavior, public API, build surface, and tests.

## [Unreleased]

### Added

- Added the initial CMake package skeleton, install export configuration, and package surface smoke test.
- Added `TraceId` and `SpanId` fixed-size identifier types with hex parsing, lowercase formatting, non-zero validation, random generation, binary accessors, and equality.
- Added Task 2 trace identifier test coverage and CTest registration.
