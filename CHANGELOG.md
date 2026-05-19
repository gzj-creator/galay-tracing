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
- Added W3C `traceparent` extraction/injection with opaque `tracestate` preservation.
- Added thread-local trace context storage, `Span`, and move-only `SpanGuard` scope restoration.
- Added a trace-aware logger facade, source location capture macros, log sink interface, and console sink.
- Added optional `SpdlogSink` adapter target that renders snapshotted trace fields without relying on MDC.
- Added a bounded batch span processor, sampler interfaces, span exporter interfaces, and JSON-lines file exporter.
- Added optional `galay-kernel` task context capture adapter for coroutine propagation.
- Added direct-include examples for automatic log correlation and W3C trace context propagation.
- Added a guarded C++23 module facade source layout for supported toolchains.
- Added release benchmark programs for disabled logs, enabled logs, span scopes, and traceparent parse/inject.
- Added structured event logging with concept-based writers, `event(ctx).info(...)`, and Rust-like `GALAY_EVENT_*` callsite macros that skip field evaluation when disabled.
- Added an OTLP/HTTP JSON span exporter with configurable endpoint, headers, custom transports, and an optional `galay-http` backed transport.
- Added OTLP JSON and structured event benchmarks, plus a Rust `tracing` comparison benchmark.

### Changed

- Switched kernel coroutine tracing from task wrapping to explicit context capture and context-aware logging APIs.
- Reduced tracing hot-path overhead by using snapshot-based logger sinks, lock-free default writer lookup, thread-local ID generation, and a slimmer `SpanGuard`.
- Split lightweight `LogContext` from full `TraceContext` so log records keep trace/span identity without copying tracestate or parent span metadata.
- Reduced logger fallback overhead by avoiding sink snapshot reference-count churn and by tightening structured-event fallback formatting.
