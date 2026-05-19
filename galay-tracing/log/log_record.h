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
class LogFieldValue {
public:
    constexpr LogFieldValue() noexcept = default;

    [[nodiscard]] static constexpr LogFieldValue fromInt64(std::int64_t value) noexcept {
        return LogFieldValue(LogFieldType::kInt64, Storage(value));
    }

    [[nodiscard]] static constexpr LogFieldValue fromUInt64(std::uint64_t value) noexcept {
        return LogFieldValue(LogFieldType::kUInt64, Storage(value));
    }

    [[nodiscard]] static constexpr LogFieldValue fromDouble(double value) noexcept {
        return LogFieldValue(LogFieldType::kDouble, Storage(value));
    }

    [[nodiscard]] static constexpr LogFieldValue fromBool(bool value) noexcept {
        return LogFieldValue(LogFieldType::kBool, Storage(value));
    }

    [[nodiscard]] static constexpr LogFieldValue fromString(std::string_view value) noexcept {
        return LogFieldValue(LogFieldType::kString, Storage(value));
    }

    [[nodiscard]] constexpr LogFieldType type() const noexcept {
        return m_type;
    }

    [[nodiscard]] constexpr std::int64_t asInt64() const noexcept {
        return m_storage.int64Value;
    }

    [[nodiscard]] constexpr std::uint64_t asUInt64() const noexcept {
        return m_storage.uint64Value;
    }

    [[nodiscard]] constexpr double asDouble() const noexcept {
        return m_storage.doubleValue;
    }

    [[nodiscard]] constexpr bool asBool() const noexcept {
        return m_storage.boolValue;
    }

    [[nodiscard]] constexpr std::string_view asString() const noexcept {
        return m_storage.stringValue;
    }

private:
    union Storage {
        constexpr Storage() noexcept
            : stringValue() {
        }

        explicit constexpr Storage(std::int64_t value) noexcept
            : int64Value(value) {
        }

        explicit constexpr Storage(std::uint64_t value) noexcept
            : uint64Value(value) {
        }

        explicit constexpr Storage(double value) noexcept
            : doubleValue(value) {
        }

        explicit constexpr Storage(bool value) noexcept
            : boolValue(value) {
        }

        explicit constexpr Storage(std::string_view value) noexcept
            : stringValue(value) {
        }

        std::int64_t int64Value;
        std::uint64_t uint64Value;
        double doubleValue;
        bool boolValue;
        std::string_view stringValue;
    };

    explicit constexpr LogFieldValue(LogFieldType type, Storage storage) noexcept
        : m_type(type),
          m_storage(storage) {
    }

    LogFieldType m_type{LogFieldType::kString};
    Storage m_storage{};
};

struct LogField {
    std::string_view name;
    LogFieldValue value;
};

[[nodiscard]] constexpr LogField field(std::string_view name, std::int64_t value) noexcept {
    return LogField{.name = name, .value = LogFieldValue::fromInt64(value)};
}

[[nodiscard]] constexpr LogField field(std::string_view name, int value) noexcept {
    return field(name, static_cast<std::int64_t>(value));
}

[[nodiscard]] constexpr LogField field(std::string_view name, std::uint64_t value) noexcept {
    return LogField{.name = name, .value = LogFieldValue::fromUInt64(value)};
}

[[nodiscard]] constexpr LogField field(std::string_view name, double value) noexcept {
    return LogField{.name = name, .value = LogFieldValue::fromDouble(value)};
}

[[nodiscard]] constexpr LogField field(std::string_view name, bool value) noexcept {
    return LogField{.name = name, .value = LogFieldValue::fromBool(value)};
}

[[nodiscard]] constexpr LogField field(std::string_view name, std::string_view value) noexcept {
    return LogField{.name = name, .value = LogFieldValue::fromString(value)};
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
