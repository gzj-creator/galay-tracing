#pragma once

#include "galay-tracing/common/span_id.h"
#include "galay-tracing/common/trace_id.h"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace galay::tracing {

// Immutable trace propagation context with optional opaque tracestate.
class TraceContext {
public:
    TraceContext() = default;

    TraceContext(TraceId traceId, SpanId spanId, std::uint8_t traceFlags = 0, std::string tracestate = {})
        : m_traceId(traceId),
          m_spanId(spanId),
          m_traceFlags(traceFlags),
          m_tracestate(std::move(tracestate)) {
    }

    [[nodiscard]] bool isValid() const noexcept {
        return m_traceId.isValid() && m_spanId.isValid();
    }

    [[nodiscard]] const TraceId& traceId() const noexcept {
        return m_traceId;
    }

    [[nodiscard]] const SpanId& spanId() const noexcept {
        return m_spanId;
    }

    [[nodiscard]] std::uint8_t traceFlags() const noexcept {
        return m_traceFlags;
    }

    [[nodiscard]] bool sampled() const noexcept {
        return (m_traceFlags & kSampledFlag) != 0;
    }

    [[nodiscard]] const std::string& tracestate() const noexcept {
        return m_tracestate;
    }

    [[nodiscard]] const std::optional<SpanId>& parentSpanId() const noexcept {
        return m_parentSpanId;
    }

    void setSpanId(SpanId spanId) noexcept {
        m_spanId = spanId;
    }

    void setParentSpanId(std::optional<SpanId> parentSpanId) {
        m_parentSpanId = std::move(parentSpanId);
    }

    void setTraceFlags(std::uint8_t traceFlags) noexcept {
        m_traceFlags = traceFlags;
    }

    void setTracestate(std::string tracestate) {
        m_tracestate = std::move(tracestate);
    }

    friend bool operator==(const TraceContext&, const TraceContext&) = default;

private:
    static constexpr std::uint8_t kSampledFlag = 0x01;

    TraceId m_traceId{};
    SpanId m_spanId{};
    std::optional<SpanId> m_parentSpanId;
    std::uint8_t m_traceFlags{0};
    std::string m_tracestate;
};

// Lightweight trace identity copied into log records. It intentionally omits
// tracestate and parent span id; propagation and exporters keep using TraceContext.
class LogContext {
public:
    LogContext() = default;

    explicit constexpr LogContext(TraceId traceId, SpanId spanId, std::uint8_t traceFlags = 0) noexcept
        : m_traceId(traceId),
          m_spanId(spanId),
          m_traceFlags(traceFlags) {
    }

    explicit LogContext(const TraceContext& context) noexcept
        : LogContext(context.traceId(), context.spanId(), context.traceFlags()) {
    }

    [[nodiscard]] bool isValid() const noexcept {
        return m_traceId.isValid() && m_spanId.isValid();
    }

    [[nodiscard]] const TraceId& traceId() const noexcept {
        return m_traceId;
    }

    [[nodiscard]] const SpanId& spanId() const noexcept {
        return m_spanId;
    }

    [[nodiscard]] std::uint8_t traceFlags() const noexcept {
        return m_traceFlags;
    }

    [[nodiscard]] bool sampled() const noexcept {
        return (m_traceFlags & kSampledFlag) != 0;
    }

    friend bool operator==(const LogContext&, const LogContext&) noexcept = default;

private:
    static constexpr std::uint8_t kSampledFlag = 0x01;

    TraceId m_traceId{};
    SpanId m_spanId{};
    std::uint8_t m_traceFlags{0};
};

[[nodiscard]] inline std::optional<LogContext> makeLogContext(const std::optional<TraceContext>& context) {
    if (!context.has_value()) {
        return std::nullopt;
    }
    return LogContext(*context);
}

[[nodiscard]] inline std::optional<LogContext> makeLogContext(std::optional<TraceContext>&& context) {
    return makeLogContext(context);
}

[[nodiscard]] inline std::optional<LogContext> makeLogContext(const TraceContext& context) {
    return LogContext(context);
}

[[nodiscard]] constexpr std::optional<LogContext> makeLogContext(std::nullopt_t) noexcept {
    return std::nullopt;
}

} // namespace galay::tracing
