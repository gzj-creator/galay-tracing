#include "galay-tracing/kernel/batch_span_processor.h"
#include "galay-tracing/kernel/span_exporter.h"

#include <cassert>
#include <chrono>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace {

class RecordingExporter final : public galay::tracing::SpanExporter {
public:
    galay::tracing::ExportResult exportSpans(std::span<const galay::tracing::Span> spans) override {
        exported.insert(exported.end(), spans.begin(), spans.end());
        return galay::tracing::ExportResult::kSuccess;
    }

    bool shutdown(std::chrono::milliseconds) override {
        shutdown_called = true;
        return true;
    }

    std::vector<galay::tracing::Span> exported;
    bool shutdown_called{false};
};

galay::tracing::Span makeSpan(std::string_view name, bool sampled = true) {
    auto context = galay::tracing::TraceContext(
        galay::tracing::TraceId::random(),
        galay::tracing::SpanId::random(),
        sampled ? 0x01 : 0x00);
    galay::tracing::Span span(std::string(name), context);
    span.end();
    return span;
}

galay::tracing::BatchSpanProcessorConfig testConfig() {
    return {
        .queue_capacity = 8,
        .max_batch_size = 8,
        .flush_interval = std::chrono::hours(1),
    };
}

void sampledSpansAreExported() {
    auto exporter = std::make_unique<RecordingExporter>();
    auto* raw = exporter.get();
    galay::tracing::BatchSpanProcessor processor(std::move(exporter), testConfig());

    processor.onEnd(makeSpan("sampled", true));

    assert(processor.forceFlush(std::chrono::seconds(1)));
    assert(raw->exported.size() == 1);
    assert(raw->exported[0].name() == "sampled");
}

void unsampledSpansAreNotExported() {
    auto exporter = std::make_unique<RecordingExporter>();
    auto* raw = exporter.get();
    galay::tracing::BatchSpanProcessor processor(std::move(exporter), testConfig());

    processor.onEnd(makeSpan("unsampled", false));

    assert(processor.forceFlush(std::chrono::seconds(1)));
    assert(raw->exported.empty());
}

void fullQueueDropsNewSpans() {
    auto config = testConfig();
    config.queue_capacity = 1;
    config.max_batch_size = 8;
    auto exporter = std::make_unique<RecordingExporter>();
    auto* raw = exporter.get();
    galay::tracing::BatchSpanProcessor processor(std::move(exporter), config);

    processor.onEnd(makeSpan("first"));
    processor.onEnd(makeSpan("second"));

    assert(processor.droppedSpanCount() == 1);
    assert(processor.forceFlush(std::chrono::seconds(1)));
    assert(raw->exported.size() == 1);
    assert(raw->exported[0].name() == "first");
}

void forceFlushExportsQueuedSpans() {
    auto exporter = std::make_unique<RecordingExporter>();
    auto* raw = exporter.get();
    galay::tracing::BatchSpanProcessor processor(std::move(exporter), testConfig());

    processor.onEnd(makeSpan("one"));
    processor.onEnd(makeSpan("two"));

    assert(processor.forceFlush(std::chrono::seconds(1)));
    assert(raw->exported.size() == 2);
}

void shutdownFlushesAndStops() {
    auto exporter = std::make_unique<RecordingExporter>();
    auto* raw = exporter.get();
    galay::tracing::BatchSpanProcessor processor(std::move(exporter), testConfig());

    processor.onEnd(makeSpan("before-shutdown"));

    assert(processor.shutdown(std::chrono::seconds(1)));
    assert(raw->shutdown_called);
    assert(raw->exported.size() == 1);

    processor.onEnd(makeSpan("after-shutdown"));
    assert(raw->exported.size() == 1);
}

} // namespace

int main() {
    sampledSpansAreExported();
    unsampledSpansAreNotExported();
    fullQueueDropsNewSpans();
    forceFlushExportsQueuedSpans();
    shutdownFlushesAndStops();
}
