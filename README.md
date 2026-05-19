# galay-tracing

`galay-tracing` is a C++23 tracing and log-correlation library for the Galay ecosystem.

It provides compact trace/span identifiers, W3C Trace Context propagation, scoped spans, trace-aware logs, structured events, optional `spdlog` and `galay-kernel` adapters, and OTLP/HTTP JSON span export.

## Current Scope

Supported now:

- `TraceId` / `SpanId` parse, format, validation, random generation.
- W3C `traceparent` extract/inject with opaque `tracestate` preservation.
- `TraceContext`, `SpanContext`, `startSpan()`, `startServerSpan()`, and move-only `SpanGuard`.
- Span kind, status, and bounded owning attributes.
- Sampling configuration with always-on/off, parent-based, and trace-id-ratio samplers.
- Trace-aware logging through `Logger`, `LogSink`, `log(ctx)`, `event(ctx)`, and `GALAY_EVENT_*` macros.
- Optional `SpdlogSink`.
- Optional explicit `galay-kernel` coroutine context capture helpers.
- `BatchSpanProcessor`, `FileSpanExporter`, and OTLP/HTTP JSON exporter with configurable endpoint, headers, timeout, resource/scope metadata, and custom transport.

Known gaps are tracked in [功能边界与路线图](docs/06-功能边界与路线图.md). The most important missing pieces are span events/links, baggage, automatic HTTP middleware, and protobuf/gRPC export.

## Build

```bash
cmake -S . -B build-dev -DBUILD_TESTING=ON -DBUILD_EXAMPLES=ON -DBUILD_BENCHMARKS=OFF
cmake --build build-dev
ctest --test-dir build-dev --output-on-failure
```

Release-style benchmark build:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DBUILD_EXAMPLES=ON -DBUILD_BENCHMARKS=ON
cmake --build build-release
```

## Documentation

- [快速开始](docs/01-快速开始.md)
- [链路上下文](docs/02-链路上下文.md)
- [日志关联](docs/03-日志关联.md)
- [导出到 Collector](docs/04-导出到Collector.md)
- [性能测试](docs/05-性能测试.md)
- [功能边界与路线图](docs/06-功能边界与路线图.md)

## Minimal Example

```cpp
#include "galay-tracing/kernel/span_guard.h"
#include "galay-tracing/log/logger.h"

int main() {
    auto span = galay::tracing::startSpan("checkout");
    GALAY_LOG_INFO("order accepted {}", 123);
}
```

When a current context exists, log records snapshot the active trace/span identity at the call site. This keeps correlation correct even when sinks or external loggers write later on another thread.
