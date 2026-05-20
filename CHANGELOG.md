# Changelog

This file records user-visible changes before they are released.

Maintenance rules:

- Keep current, unreleased work under `## [Unreleased]`.
- Move accumulated entries into `## [vX.Y.Z] - YYYY-MM-DD` only when creating a release tag.
- Use semantic versioning: major for breaking or architectural changes, minor for new features, patch for fixes and small maintenance.
- Keep entries concise and focused on behavior, public API, build surface, and tests.

## [Unreleased]

## [v0.2.0] - 2026-05-20

### Added

- Added `galay::tracing::log::set/get` for tracing-internal diagnostics backed by `galay-kernel` `BaseLogger`.
- Added `TRACING_LOG_*` macros and internal diagnostics for batch span processing and OTLP/HTTP export failure paths.
- Added `T10-base_logger` coverage to verify disabled and level-filtered diagnostics do not evaluate formatting arguments.

### Changed

- Kept tracing business correlation logs on the existing `Logger` / `LogSink` / `GALAY_LOG_*` model while separating internal library diagnostics into `BaseLogger`.
- Bumped the CMake package version to `0.2.0`.
- Required `galay-kernel 5.0.0` and aligned the optional OTLP/HTTP dependency to `galay-http 3.1.0`.

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
- Added `GALAY_EVENT_*_DEFAULT` macros for default-writer structured events with disabled-field short-circuiting.
- Added lightweight `SpanContext` for span hot paths that do not need tracestate storage.
- Added `SpanTimingPolicy` configuration so span start/end timestamps can stay disabled by default and be enabled when exporters need them.
- Added an OTLP/HTTP JSON span exporter with configurable endpoint, headers, custom transports, and an optional `galay-http` backed transport.
- Added OTLP JSON and structured event benchmarks, plus a Rust `tracing` comparison benchmark.
- Added exporter-level resource attributes and instrumentation scope metadata for OTLP/HTTP JSON export.

### Changed

- Switched kernel coroutine tracing from task wrapping to explicit context capture and context-aware logging APIs.
- Reduced tracing hot-path overhead by using snapshot-based logger sinks, lock-free default writer lookup, thread-local ID generation, and a slimmer `SpanGuard`.
- Split lightweight `LogContext` from full `TraceContext` so log records keep trace/span identity without copying tracestate or parent span metadata.
- Reduced logger fallback overhead by avoiding sink snapshot reference-count churn and by tightening structured-event fallback formatting.
- Reduced structured event field footprint by compacting `LogFieldValue` storage and added benchmark output for default-writer macros and subscriber-like noop span scopes.
- Reduced default-writer event overhead by routing default `log(ctx)` and `event(ctx)` through a compact pointer-sized writer proxy.
- Reduced default span scope overhead by keeping span start/end timestamp recording disabled unless `SpanTimingPolicy::kEnabled` is configured.
- Reduced default structured-event overhead by inlining the configured default writer lookup and structured writer fast path.
- Reduced real span scope overhead by storing `SpanContext` inside `Span` and generating IDs from a faster thread-local non-cryptographic stream.
- `Span::context()` now returns a full `TraceContext` by value; exporters can use `spanContext()` and `tracestate()` to avoid rebuilding propagation context on hot paths.
- Added span kind, status, and bounded owning attributes, and encoded them in OTLP/HTTP JSON export.
- Integrated process-wide sampler configuration with parent-based and trace-id-ratio sampling decisions at span start.
