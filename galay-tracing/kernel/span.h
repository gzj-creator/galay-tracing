#pragma once

#include "galay-tracing/context/trace_context.h"

#include <chrono>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace galay::tracing {

enum class SpanTimingPolicy {
    kDisabled,
    kEnabled,
};

enum class SpanKind {
    kInternal,
    kServer,
    kClient,
    kProducer,
    kConsumer,
};

enum class SpanStatusCode {
    kUnset,
    kOk,
    kError,
};

enum class SpanAttributeType {
    kInt64,
    kUInt64,
    kDouble,
    kBool,
    kString,
};

// Owning span attribute value. Span attributes may be exported asynchronously,
// so string values are copied instead of borrowed from the call site.
class SpanAttributeValue {
public:
    SpanAttributeValue() = default;

    [[nodiscard]] static SpanAttributeValue fromInt64(std::int64_t value);
    [[nodiscard]] static SpanAttributeValue fromUInt64(std::uint64_t value);
    [[nodiscard]] static SpanAttributeValue fromDouble(double value);
    [[nodiscard]] static SpanAttributeValue fromBool(bool value);
    [[nodiscard]] static SpanAttributeValue fromString(std::string value);

    [[nodiscard]] SpanAttributeType type() const noexcept;
    [[nodiscard]] std::int64_t asInt64() const;
    [[nodiscard]] std::uint64_t asUInt64() const;
    [[nodiscard]] double asDouble() const;
    [[nodiscard]] bool asBool() const;
    [[nodiscard]] const std::string& asString() const;

private:
    using Storage = std::variant<std::int64_t, std::uint64_t, double, bool, std::string>;

    explicit SpanAttributeValue(Storage storage);

    Storage m_storage{std::string()};
};

struct SpanAttribute {
    std::string name;
    SpanAttributeValue value;
};

[[nodiscard]] SpanAttribute spanAttribute(std::string_view name, std::int64_t value);
[[nodiscard]] SpanAttribute spanAttribute(std::string_view name, int value);
[[nodiscard]] SpanAttribute spanAttribute(std::string_view name, std::uint64_t value);
[[nodiscard]] SpanAttribute spanAttribute(std::string_view name, double value);
[[nodiscard]] SpanAttribute spanAttribute(std::string_view name, bool value);
[[nodiscard]] SpanAttribute spanAttribute(std::string_view name, std::string_view value);
[[nodiscard]] SpanAttribute spanAttribute(std::string_view name, const char* value);

struct SpanStatus {
    SpanStatusCode code{SpanStatusCode::kUnset};
    std::string message;
};

// Controls whether spans record start/end timestamps. Disabled by default so
// log timestamps remain the cheap default timing source.
void setSpanTimingPolicy(SpanTimingPolicy policy) noexcept;
[[nodiscard]] SpanTimingPolicy spanTimingPolicy() noexcept;

class Span {
public:
    using Clock = std::chrono::steady_clock;
    static constexpr std::size_t kMaxAttributes = 32;

    Span() = default;
    Span(std::string name, TraceContext context);
    Span(std::string name, TraceContext context, SpanTimingPolicy timingPolicy);
    Span(std::string name, SpanContext context, std::string tracestate = {});
    Span(std::string name, SpanContext context, std::string tracestate, SpanTimingPolicy timingPolicy);

    [[nodiscard]] std::string_view name() const noexcept {
        return m_name;
    }

    [[nodiscard]] TraceContext context() const {
        return m_context.toTraceContext(m_tracestate);
    }

    [[nodiscard]] const SpanContext& spanContext() const noexcept {
        return m_context;
    }

    [[nodiscard]] const std::string& tracestate() const noexcept {
        return m_tracestate;
    }

    [[nodiscard]] SpanKind kind() const noexcept {
        return m_kind;
    }

    void setKind(SpanKind kind) noexcept {
        m_kind = kind;
    }

    [[nodiscard]] const SpanStatus& status() const noexcept {
        return m_status;
    }

    void setStatus(SpanStatusCode code, std::string message = {});

    [[nodiscard]] std::span<const SpanAttribute> attributes() const noexcept {
        return std::span<const SpanAttribute>(m_attributes.data(), m_attributes.size());
    }

    [[nodiscard]] bool setAttribute(SpanAttribute attribute);
    [[nodiscard]] bool setAttribute(std::string_view name, std::int64_t value);
    [[nodiscard]] bool setAttribute(std::string_view name, int value);
    [[nodiscard]] bool setAttribute(std::string_view name, std::uint64_t value);
    [[nodiscard]] bool setAttribute(std::string_view name, double value);
    [[nodiscard]] bool setAttribute(std::string_view name, bool value);
    [[nodiscard]] bool setAttribute(std::string_view name, std::string_view value);
    [[nodiscard]] bool setAttribute(std::string_view name, const char* value);

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
    SpanContext m_context;
    std::string m_tracestate;
    SpanKind m_kind{SpanKind::kInternal};
    SpanStatus m_status;
    std::vector<SpanAttribute> m_attributes;
    Clock::time_point m_startedAt{};
    Clock::time_point m_endedAt{};
    bool m_ended{false};
};

} // namespace galay::tracing
