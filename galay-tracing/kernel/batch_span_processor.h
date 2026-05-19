#pragma once

#include "galay-tracing/kernel/span_exporter.h"
#include "galay-tracing/kernel/span_processor.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace galay::tracing {

struct BatchSpanProcessorConfig {
    std::size_t queue_capacity{2048};
    std::size_t max_batch_size{512};
    std::chrono::milliseconds flush_interval{std::chrono::milliseconds(5000)};
};

class BatchSpanProcessor final : public SpanProcessor {
public:
    explicit BatchSpanProcessor(std::unique_ptr<SpanExporter> exporter, BatchSpanProcessorConfig config = {});
    ~BatchSpanProcessor() noexcept override;

    BatchSpanProcessor(const BatchSpanProcessor&) = delete;
    BatchSpanProcessor& operator=(const BatchSpanProcessor&) = delete;

    void onEnd(Span span) override;
    bool forceFlush(std::chrono::milliseconds timeout) override;
    bool shutdown(std::chrono::milliseconds timeout) override;

    [[nodiscard]] std::size_t droppedSpanCount() const noexcept;

private:
    void workerLoop();
    [[nodiscard]] std::vector<Span> drainQueue(std::size_t maxCount);
    [[nodiscard]] bool exportBatch(std::span<const Span> spans);

    std::unique_ptr<SpanExporter> m_exporter;
    BatchSpanProcessorConfig m_config;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::deque<Span> m_queue;
    std::mutex m_exportMutex;
    std::thread m_worker;
    std::atomic<std::size_t> m_droppedSpans{0};
    bool m_shutdown{false};
    bool m_exporterShutdown{false};
};

} // namespace galay::tracing
