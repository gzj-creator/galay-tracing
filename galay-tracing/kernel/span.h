#pragma once

#include "galay-tracing/context/trace_context.h"

#include <chrono>
#include <string>
#include <string_view>

namespace galay::tracing {

class Span {
public:
    using Clock = std::chrono::steady_clock;

    Span() = default;
    Span(std::string name, TraceContext context);

    [[nodiscard]] std::string_view name() const noexcept {
        return m_name;
    }

    [[nodiscard]] const TraceContext& context() const noexcept {
        return m_context;
    }

    [[nodiscard]] Clock::time_point startedAt() const noexcept {
        return m_startedAt;
    }

    [[nodiscard]] Clock::time_point endedAt() const noexcept {
        return m_endedAt;
    }

    [[nodiscard]] bool ended() const noexcept {
        return m_ended;
    }

    void end() noexcept;

private:
    std::string m_name;
    TraceContext m_context;
    Clock::time_point m_startedAt{};
    Clock::time_point m_endedAt{};
    bool m_ended{false};
};

} // namespace galay::tracing
