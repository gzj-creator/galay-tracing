#include "galay-tracing/context/context_storage.h"
#include "galay-tracing/kernel/span_guard.h"

#include <chrono>
#include <iostream>

namespace {

[[nodiscard]] const char* buildType() {
#ifdef NDEBUG
    return "Release";
#else
    return "Debug";
#endif
}

double measureChildSpanNs(bool sampled) {
    constexpr int kIterations = 20000;
    const auto parent = galay::tracing::TraceContext(
        galay::tracing::TraceId::random(),
        galay::tracing::SpanId::random(),
        sampled ? 0x01 : 0x00);
    galay::tracing::setCurrentContext(parent);

    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < kIterations; ++i) {
        auto span = galay::tracing::startSpan("bench");
    }
    const auto elapsed = std::chrono::steady_clock::now() - start;
    galay::tracing::clearCurrentContext();

    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
    return static_cast<double>(ns) / kIterations;
}

} // namespace

int main() {
    std::cout << "B3-SpanScope workload=20000 build=" << buildType() << " backend=core"
              << " sampled_ns_per_scope=" << measureChildSpanNs(true)
              << " unsampled_ns_per_scope=" << measureChildSpanNs(false) << '\n';
}
