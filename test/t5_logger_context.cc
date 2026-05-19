#include "galay-tracing/context/context_storage.h"
#include "galay-tracing/log/logger.h"
#include "galay-tracing/log/log_sink.h"

#include <cassert>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct ExplodingFormat {};

} // namespace

template <>
struct std::formatter<ExplodingFormat> {
    constexpr auto parse(std::format_parse_context& context) {
        return context.begin();
    }

    auto format(const ExplodingFormat&, std::format_context& context) const {
        throw std::runtime_error("disabled log level formatted an argument");
        return context.out();
    }
};

namespace {

class TestSink final : public galay::tracing::LogSink {
public:
    void write(const galay::tracing::LogRecord& record) override {
        records.push_back(record);
    }

    std::vector<galay::tracing::LogRecord> records;
};

void noContextLogsStillEmit() {
    galay::tracing::clearCurrentContext();
    galay::tracing::Logger logger;
    auto sink = std::make_shared<TestSink>();
    logger.clearSinks();
    logger.addSink(sink);

    logger.log(galay::tracing::LogLevel::kInfo, {"test.cc", 42, "noContextLogsStillEmit"}, "hello {}", "world");

    assert(sink->records.size() == 1);
    assert(sink->records[0].message == "hello world");
    assert(!sink->records[0].context.has_value());
    assert(sink->records[0].source.line == 42);
}

void contextLogsIncludeTraceAndSpanIds() {
    auto context = galay::tracing::TraceContext(
        galay::tracing::TraceId::fromHex("4bf92f3577b34da6a3ce929d0e0e4736"),
        galay::tracing::SpanId::fromHex("00f067aa0ba902b7"),
        0x01);
    galay::tracing::setCurrentContext(context);

    galay::tracing::Logger logger;
    auto sink = std::make_shared<TestSink>();
    logger.clearSinks();
    logger.addSink(sink);

    logger.log(galay::tracing::LogLevel::kInfo, {"test.cc", 7, "contextLogsIncludeTraceAndSpanIds"}, "accepted");

    assert(sink->records.size() == 1);
    assert(sink->records[0].context.has_value());
    assert(sink->records[0].context->traceId() == context.traceId());
    assert(sink->records[0].context->spanId() == context.spanId());

    galay::tracing::clearCurrentContext();
}

void disabledLevelDoesNotFormat() {
    galay::tracing::Logger logger;
    auto sink = std::make_shared<TestSink>();
    logger.clearSinks();
    logger.addSink(sink);
    logger.setLevel(galay::tracing::LogLevel::kWarn);

    logger.log(galay::tracing::LogLevel::kDebug, {"test.cc", 11, "disabledLevelDoesNotFormat"}, "{}", ExplodingFormat{});

    assert(sink->records.empty());
}

void macrosCaptureFileAndLine() {
    galay::tracing::clearCurrentContext();
    galay::tracing::Logger logger;
    auto sink = std::make_shared<TestSink>();
    logger.clearSinks();
    logger.addSink(sink);
    galay::tracing::setDefaultLogger(&logger);

    const auto expectedLine = __LINE__ + 1;
    GALAY_LOG_INFO("macro {}", 7);

    galay::tracing::setDefaultLogger(nullptr);

    assert(sink->records.size() == 1);
    assert(sink->records[0].message == "macro 7");
    assert(std::string_view(sink->records[0].source.file).ends_with("t5_logger_context.cc"));
    assert(sink->records[0].source.line == expectedLine);
}

} // namespace

int main() {
    noContextLogsStillEmit();
    contextLogsIncludeTraceAndSpanIds();
    disabledLevelDoesNotFormat();
    macrosCaptureFileAndLine();
}
