#include "galay-tracing/context/context_storage.h"

namespace galay::tracing {

namespace {

thread_local std::optional<TraceContext> t_currentContext;

} // namespace

std::optional<TraceContext> currentContext() noexcept {
    return t_currentContext;
}

void setCurrentContext(std::optional<TraceContext> context) {
    t_currentContext = std::move(context);
}

void clearCurrentContext() noexcept {
    t_currentContext.reset();
}

} // namespace galay::tracing
