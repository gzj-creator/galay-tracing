#include "galay-tracing/adapters/spdlog_sink.h"

#include "galay-tracing/log/log_level.h"

#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include <string>

namespace galay::tracing {

namespace {

[[nodiscard]] spdlog::level::level_enum toSpdlogLevel(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::kTrace:
        return spdlog::level::trace;
    case LogLevel::kDebug:
        return spdlog::level::debug;
    case LogLevel::kInfo:
        return spdlog::level::info;
    case LogLevel::kWarn:
        return spdlog::level::warn;
    case LogLevel::kError:
        return spdlog::level::err;
    case LogLevel::kOff:
        return spdlog::level::off;
    }
    return spdlog::level::info;
}

[[nodiscard]] std::string renderRecord(const LogRecord& record) {
    std::string line;
    line.append("level=");
    line.append(logLevelName(record.level));
    if (record.context.has_value()) {
        line.append(" trace_id=");
        line.append(record.context->traceId().toHex());
        line.append(" span_id=");
        line.append(record.context->spanId().toHex());
    }
    line.append(" file=");
    line.append(record.source.file);
    line.push_back(':');
    line.append(std::to_string(record.source.line));
    line.append(" msg=\"");
    line.append(record.message);
    line.push_back('"');
    return line;
}

} // namespace

SpdlogSink::SpdlogSink(std::shared_ptr<spdlog::logger> logger)
    : m_logger(std::move(logger)) {
}

void SpdlogSink::write(const LogRecord& record) {
    if (!m_logger) {
        return;
    }

    m_logger->log(toSpdlogLevel(record.level), "{}", renderRecord(record));
}

} // namespace galay::tracing
