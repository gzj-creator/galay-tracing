#include "galay-tracing/kernel/batch_span_processor.h"

#include <algorithm>
#include <utility>

namespace galay::tracing {

namespace {

[[nodiscard]] BatchSpanProcessorConfig normalizeConfig(BatchSpanProcessorConfig config) noexcept {
    config.queue_capacity = std::max<std::size_t>(config.queue_capacity, 1);
    config.max_batch_size = std::clamp<std::size_t>(config.max_batch_size, 1, config.queue_capacity);
    if (config.flush_interval <= std::chrono::milliseconds::zero()) {
        config.flush_interval = std::chrono::milliseconds(1);
    }
    return config;
}

} // namespace

bool SpanExporter::forceFlush(std::chrono::milliseconds) {
    return true;
}

bool SpanExporter::shutdown(std::chrono::milliseconds) {
    return true;
}

BatchSpanProcessor::BatchSpanProcessor(std::unique_ptr<SpanExporter> exporter, BatchSpanProcessorConfig config)
    : m_exporter(std::move(exporter)),
      m_config(normalizeConfig(config)),
      m_worker(&BatchSpanProcessor::workerLoop, this) {
}

BatchSpanProcessor::~BatchSpanProcessor() noexcept {
    static_cast<void>(shutdown(std::chrono::milliseconds(500)));
}

void BatchSpanProcessor::onEnd(Span span) {
    if (!span.spanContext().sampled()) {
        return;
    }

    bool shouldNotify = false;
    {
        std::lock_guard lock(m_mutex);
        if (m_shutdown) {
            return;
        }

        if (m_queue.size() >= m_config.queue_capacity) {
            m_droppedSpans.fetch_add(1, std::memory_order_relaxed);
            return;
        }

        m_queue.push_back(std::move(span));
        shouldNotify = m_queue.size() >= m_config.max_batch_size;
    }

    if (shouldNotify) {
        m_condition.notify_one();
    }
}

bool BatchSpanProcessor::forceFlush(std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    bool ok = true;

    while (std::chrono::steady_clock::now() <= deadline) {
        auto batch = drainQueue(m_config.max_batch_size);
        if (batch.empty()) {
            break;
        }
        ok = exportBatch(batch) && ok;
    }

    if (m_exporter) {
        std::lock_guard exportLock(m_exportMutex);
        ok = m_exporter->forceFlush(timeout) && ok;
    }
    return ok && std::chrono::steady_clock::now() <= deadline;
}

bool BatchSpanProcessor::shutdown(std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    bool firstShutdown = false;

    {
        std::lock_guard lock(m_mutex);
        firstShutdown = !m_shutdown;
        m_shutdown = true;
    }
    m_condition.notify_one();

    bool ok = forceFlush(timeout);

    if (firstShutdown && m_worker.joinable()) {
        m_worker.join();
    }

    if (m_exporter && !m_exporterShutdown) {
        std::lock_guard exportLock(m_exportMutex);
        ok = m_exporter->shutdown(timeout) && ok;
        m_exporterShutdown = true;
    }

    return ok && std::chrono::steady_clock::now() <= deadline;
}

std::size_t BatchSpanProcessor::droppedSpanCount() const noexcept {
    return m_droppedSpans.load(std::memory_order_relaxed);
}

void BatchSpanProcessor::workerLoop() {
    while (true) {
        {
            std::unique_lock lock(m_mutex);
            m_condition.wait_for(lock, m_config.flush_interval, [&] {
                return m_shutdown || m_queue.size() >= m_config.max_batch_size;
            });

            if (m_shutdown && m_queue.empty()) {
                return;
            }
        }

        auto batch = drainQueue(m_config.max_batch_size);
        if (!batch.empty()) {
            static_cast<void>(exportBatch(batch));
        }
    }
}

std::vector<Span> BatchSpanProcessor::drainQueue(std::size_t maxCount) {
    std::vector<Span> batch;
    std::lock_guard lock(m_mutex);
    const auto count = std::min(maxCount, m_queue.size());
    batch.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        batch.push_back(std::move(m_queue.front()));
        m_queue.pop_front();
    }
    return batch;
}

bool BatchSpanProcessor::exportBatch(std::span<const Span> spans) {
    if (spans.empty() || !m_exporter) {
        return true;
    }

    std::lock_guard exportLock(m_exportMutex);
    return m_exporter->exportSpans(spans) == ExportResult::kSuccess;
}

} // namespace galay::tracing
