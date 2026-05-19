#pragma once

#include "galay-tracing/kernel/span.h"

#include <optional>
#include <string_view>

namespace galay::tracing {

class SpanGuard {
public:
    SpanGuard() noexcept = default;
    SpanGuard(Span span, std::optional<TraceContext> previousContext);
    ~SpanGuard() noexcept;

    SpanGuard(const SpanGuard&) = delete;
    SpanGuard& operator=(const SpanGuard&) = delete;

    SpanGuard(SpanGuard&& other) noexcept;
    SpanGuard& operator=(SpanGuard&& other) noexcept;

    [[nodiscard]] const Span& span() const noexcept {
        return *m_span;
    }

    [[nodiscard]] bool active() const noexcept {
        return m_active;
    }

    void end() noexcept;

private:
    void restore() noexcept;

    std::optional<Span> m_span;
    std::optional<TraceContext> m_previousContext;
    bool m_active{false};
};

[[nodiscard]] SpanGuard startSpan(std::string_view name);
[[nodiscard]] SpanGuard startServerSpan(std::string_view name, const TraceContext& parent);

} // namespace galay::tracing
