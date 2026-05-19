# galay-tracing

`galay-tracing` is a C++23 tracing and logging correlation library for the
Galay ecosystem.

This repository currently contains the initial CMake package skeleton. The
temporary `TraceId` surface is intentionally minimal and will be expanded by
the trace identifier task.

## Build

```bash
cmake -S . -B build-dev -DBUILD_TESTING=ON -DBUILD_EXAMPLES=OFF -DBUILD_BENCHMARKS=OFF
cmake --build build-dev
ctest --test-dir build-dev --output-on-failure
```
