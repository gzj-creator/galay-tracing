#pragma once

#include "galay-tracing/common/source_location.h"
#include "galay-tracing/context/trace_context.h"
#include "galay-tracing/log/log_level.h"

#include <chrono>
#include <optional>
#include <string>

namespace galay::tracing {

struct LogRecord {
    LogLevel level{LogLevel::kInfo};
    std::string message;
    SourceLocation source;
    std::optional<TraceContext> context;
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
};

} // namespace galay::tracing
