#pragma once

#include "galay-tracing/context/trace_context.h"

#include <expected>
#include <string>
#include <string_view>

namespace galay::tracing {

enum class TraceparentError {
    kMalformed,
    kUnsupportedVersion,
    kInvalidTraceId,
    kInvalidSpanId,
    kInvalidFlags,
};

// Parses a W3C traceparent header and preserves tracestate as an opaque string.
[[nodiscard]] std::expected<TraceContext, TraceparentError> extractTraceparent(
    std::string_view value,
    std::string_view tracestate = {});

// Formats a valid context as a lowercase W3C traceparent header. Invalid input returns an empty string.
[[nodiscard]] std::string injectTraceparent(const TraceContext& context);

// Returns the context's opaque tracestate value for outbound propagation.
[[nodiscard]] std::string injectTracestate(const TraceContext& context);

} // namespace galay::tracing
