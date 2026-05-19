#pragma once

#include "galay-tracing/context/context_storage.h"

#include <optional>
#include <utility>

namespace galay::tracing {

// Scoped bridge for non-suspending kernel callbacks. Do not hold this object
// across co_await/co_yield; pass the captured context explicitly instead.
class KernelTraceContextScope {
public:
    explicit KernelTraceContextScope(std::optional<TraceContext> context)
        : m_previous(currentContext()) {
        setCurrentContext(std::move(context));
    }

    ~KernelTraceContextScope() noexcept {
        try {
            setCurrentContext(std::move(m_previous));
        } catch (...) {
            clearCurrentContext();
        }
    }

    KernelTraceContextScope(const KernelTraceContextScope&) = delete;
    KernelTraceContextScope& operator=(const KernelTraceContextScope&) = delete;

private:
    std::optional<TraceContext> m_previous;
};

// Snapshots the current thread context before handing work to galay-kernel.
// Coroutine tasks should carry this value as an argument and use *_CTX log APIs.
[[nodiscard]] inline std::optional<TraceContext> captureTraceContext() noexcept {
    return currentContext();
}

} // namespace galay::tracing
