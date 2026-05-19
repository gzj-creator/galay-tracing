#pragma once

#include "galay-tracing/context/trace_context.h"

#include <optional>
#include <string>

namespace galay::tracing {

namespace detail {

struct CurrentContextState {
    std::optional<SpanContext> spanContext;
    std::string tracestate;
};

[[nodiscard]] CurrentContextState currentContextState();
void setCurrentContextState(CurrentContextState state);

} // namespace detail

// Returns the current thread's active trace context, if one is set.
[[nodiscard]] std::optional<TraceContext> currentContext() noexcept;

// Sets the current thread's active trace context.
void setCurrentContext(std::optional<TraceContext> context);

inline void setCurrentContext(const TraceContext& context) {
    setCurrentContext(std::optional<TraceContext>(context));
}

// Clears the current thread's active trace context.
void clearCurrentContext() noexcept;

} // namespace galay::tracing
