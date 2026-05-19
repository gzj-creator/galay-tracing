#pragma once

#include "galay-tracing/context/context_storage.h"

#include "galay-kernel/kernel/task.h"

#include <optional>
#include <type_traits>
#include <utility>

namespace galay::tracing {

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

template <typename T>
[[nodiscard]] galay::kernel::Task<T> withTraceContext(
    std::optional<TraceContext> context,
    galay::kernel::Task<T> task) {
    KernelTraceContextScope scope(std::move(context));
    if constexpr (std::is_void_v<T>) {
        co_await std::move(task);
        co_return;
    } else {
        co_return co_await std::move(task);
    }
}

template <typename T>
[[nodiscard]] galay::kernel::Task<T> withCapturedTraceContext(galay::kernel::Task<T> task) {
    return withTraceContext(currentContext(), std::move(task));
}

} // namespace galay::tracing
