#pragma once

#include "galay-tracing/common/source_location.h"
#include "galay-tracing/context/trace_context.h"
#include "galay-tracing/log/log_level.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace galay::tracing {

enum class LogFieldType {
    kInt64,
    kUInt64,
    kDouble,
    kBool,
    kString,
};

// Non-owning structured field value used by event(...). String values must
// outlive the write call that receives the containing StructuredLogRecord.
struct LogFieldValue {
    LogFieldType type{LogFieldType::kString};
    std::int64_t int64_value{0};
    std::uint64_t uint64_value{0};
    double double_value{0.0};
    bool bool_value{false};
    std::string_view string_value;

    [[nodiscard]] constexpr std::int64_t asInt64() const noexcept {
        return int64_value;
    }
};

struct LogField {
    std::string_view name;
    LogFieldValue value;
};

[[nodiscard]] constexpr LogField field(std::string_view name, std::int64_t value) noexcept {
    return LogField{.name = name, .value = {.type = LogFieldType::kInt64, .int64_value = value}};
}

[[nodiscard]] constexpr LogField field(std::string_view name, int value) noexcept {
    return field(name, static_cast<std::int64_t>(value));
}

[[nodiscard]] constexpr LogField field(std::string_view name, std::uint64_t value) noexcept {
    return LogField{.name = name, .value = {.type = LogFieldType::kUInt64, .uint64_value = value}};
}

[[nodiscard]] constexpr LogField field(std::string_view name, double value) noexcept {
    return LogField{.name = name, .value = {.type = LogFieldType::kDouble, .double_value = value}};
}

[[nodiscard]] constexpr LogField field(std::string_view name, bool value) noexcept {
    return LogField{.name = name, .value = {.type = LogFieldType::kBool, .bool_value = value}};
}

[[nodiscard]] constexpr LogField field(std::string_view name, std::string_view value) noexcept {
    return LogField{.name = name, .value = {.type = LogFieldType::kString, .string_value = value}};
}

[[nodiscard]] constexpr LogField field(std::string_view name, const char* value) noexcept {
    return field(name, std::string_view(value == nullptr ? "" : value));
}

struct LogRecord {
    LogLevel level{LogLevel::kInfo};
    std::string message;
    SourceLocation source;
    std::optional<LogContext> context;
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
};

// Allocation-free structured event record. Field storage is borrowed from the
// logging call and is only valid until the writer's write() call returns.
struct StructuredLogRecord {
    LogLevel level{LogLevel::kInfo};
    std::string_view name;
    std::span<const LogField> fields;
    SourceLocation source;
    std::optional<LogContext> context;
};

} // namespace galay::tracing
