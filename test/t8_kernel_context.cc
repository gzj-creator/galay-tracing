#include "galay-tracing/adapters/kernel_context.h"

#include "galay-tracing/context/context_storage.h"
#include "galay-tracing/log/logger.h"
#include "galay-tracing/log/log_sink.h"

#include "galay-kernel/kernel/runtime.h"
#include "galay-kernel/kernel/task.h"

#include <cassert>
#include <memory>
#include <vector>

namespace {

class TestSink final : public galay::tracing::LogSink {
public:
    void write(const galay::tracing::LogRecord& record) override {
        records.push_back(record);
    }

    std::vector<galay::tracing::LogRecord> records;
};

galay::kernel::Task<void> logInsideKernelTask() {
    GALAY_LOG_INFO("inside kernel task");
    co_return;
}

} // namespace

int main() {
    auto sink = std::make_shared<TestSink>();
    galay::tracing::Logger logger;
    logger.clearSinks();
    logger.addSink(sink);
    galay::tracing::setDefaultLogger(&logger);

    const auto context = galay::tracing::TraceContext(
        galay::tracing::TraceId::fromHex("4bf92f3577b34da6a3ce929d0e0e4736"),
        galay::tracing::SpanId::fromHex("00f067aa0ba902b7"),
        0x01);
    galay::tracing::setCurrentContext(context);
    auto task = galay::tracing::withCapturedTraceContext(logInsideKernelTask());
    galay::tracing::clearCurrentContext();

    auto runtime = galay::kernel::RuntimeBuilder()
        .ioSchedulerCount(0)
        .computeSchedulerCount(1)
        .build();
    runtime.blockOn(std::move(task));

    galay::tracing::setDefaultLogger(nullptr);

    assert(sink->records.size() == 1);
    assert(sink->records[0].context.has_value());
    assert(sink->records[0].context->traceId() == context.traceId());
    assert(sink->records[0].context->spanId() == context.spanId());
}
