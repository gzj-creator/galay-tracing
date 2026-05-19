#pragma once

#include "galay-tracing/common/source_location.h"
#include "galay-tracing/context/context_storage.h"
#include "galay-tracing/log/log_level.h"
#include "galay-tracing/log/log_record.h"
#include "galay-tracing/log/log_sink.h"

#include <atomic>
#include <array>
#include <concepts>
#include <cstddef>
#include <format>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace galay::tracing {

// Compile-time writer contract for tracing log records.
template <typename Writer>
concept LogWriter = requires(Writer& writer, const Writer& constWriter, LogLevel level, LogRecord record) {
    { constWriter.isEnabled(level) } noexcept -> std::convertible_to<bool>;
    writer.write(std::move(record));
};

template <typename Writer>
concept StructuredLogWriter = requires(Writer& writer, const Writer& constWriter, LogLevel level, StructuredLogRecord record) {
    { constWriter.isEnabled(level) } noexcept -> std::convertible_to<bool>;
    writer.write(record);
};

namespace detail {

struct ErasedLogWriter {
    using IsEnabledFn = bool (*)(const void*, LogLevel) noexcept;
    using WriteFn = void (*)(void*, LogRecord);
    using WriteStructuredFn = void (*)(void*, StructuredLogRecord);

    void* object = nullptr;
    IsEnabledFn isEnabledFn = nullptr;
    WriteFn writeFn = nullptr;
    WriteStructuredFn writeStructuredFn = nullptr;

    [[nodiscard]] bool isEnabled(LogLevel level) const noexcept {
        return object != nullptr && isEnabledFn != nullptr && isEnabledFn(object, level);
    }

    void write(LogRecord record) {
        if (object == nullptr) {
            return;
        }
        if (writeFn != nullptr) {
            writeFn(object, std::move(record));
            return;
        }
        if (writeStructuredFn != nullptr) {
            writeStructuredFn(object, StructuredLogRecord{
                .level = record.level,
                .name = record.message,
                .fields = {},
                .source = record.source,
                .context = std::move(record.context),
            });
        }
    }

    void write(StructuredLogRecord record);
};

template <typename Writer>
    requires(LogWriter<Writer> || StructuredLogWriter<Writer>)
class BorrowedLogWriter {
public:
    explicit BorrowedLogWriter(Writer& writer) noexcept
        : m_writer(&writer) {
    }

    [[nodiscard]] bool isEnabled(LogLevel level) const noexcept {
        return m_writer != nullptr && m_writer->isEnabled(level);
    }

    void write(LogRecord record)
        requires LogWriter<Writer> {
        if (m_writer != nullptr) {
            m_writer->write(std::move(record));
        }
    }

    void write(StructuredLogRecord record)
        requires StructuredLogWriter<Writer> {
        if (m_writer != nullptr) {
            m_writer->write(record);
        }
    }

private:
    Writer* m_writer;
};

template <typename Writer>
[[nodiscard]] constexpr ErasedLogWriter::WriteFn logWriteFn() noexcept {
    if constexpr (LogWriter<Writer>) {
        return [](void* object, LogRecord record) {
            static_cast<Writer*>(object)->write(std::move(record));
        };
    } else {
        return nullptr;
    }
}

template <typename Writer>
[[nodiscard]] constexpr ErasedLogWriter::WriteStructuredFn structuredLogWriteFn() noexcept {
    if constexpr (StructuredLogWriter<Writer>) {
        return [](void* object, StructuredLogRecord record) {
            static_cast<Writer*>(object)->write(record);
        };
    } else {
        return nullptr;
    }
}

template <LogLevel kLevel, StructuredLogWriter Writer, typename... Fields>
void writeEventUnchecked(
    Writer& writer,
    std::optional<TraceContext> context,
    SourceLocation source,
    std::string_view name,
    Fields&&... fields) {
    std::array<LogField, sizeof...(Fields)> fieldArray{std::forward<Fields>(fields)...};
    writer.write(StructuredLogRecord{
        .level = kLevel,
        .name = name,
        .fields = std::span<const LogField>(fieldArray.data(), fieldArray.size()),
        .source = source,
        .context = std::move(context),
    });
}

void setDefaultLogWriterRef(ErasedLogWriter writer) noexcept;
[[nodiscard]] ErasedLogWriter defaultLogWriterRef() noexcept;

} // namespace detail

class Logger {
public:
    explicit Logger(LogLevel level = LogLevel::kTrace) noexcept;

    void setLevel(LogLevel level) noexcept;
    [[nodiscard]] LogLevel level() const noexcept;
    [[nodiscard]] bool isEnabled(LogLevel level) const noexcept;

    void addSink(std::shared_ptr<LogSink> sink);
    void clearSinks();

    template <typename... Args>
    void log(LogLevel level, SourceLocation source, std::format_string<Args...> fmt, Args&&... args) {
        if (!isEnabled(level)) {
            return;
        }

        publish(LogRecord{
            .level = level,
            .message = std::format(fmt, std::forward<Args>(args)...),
            .source = source,
            .context = currentContext(),
        });
    }

    // Logs with an explicitly carried context. This is the coroutine-safe
    // entrypoint for tasks that cannot rely on thread-local currentContext().
    template <typename... Args>
    void logWithContext(
        LogLevel level,
        SourceLocation source,
        std::optional<TraceContext> context,
        std::format_string<Args...> fmt,
        Args&&... args) {
        if (!isEnabled(level)) {
            return;
        }

        publish(LogRecord{
            .level = level,
            .message = std::format(fmt, std::forward<Args>(args)...),
            .source = source,
            .context = std::move(context),
        });
    }

    void write(LogRecord record);
    void write(StructuredLogRecord record);

private:
    struct SinkSnapshot {
        std::vector<std::shared_ptr<LogSink>> sinks;
    };

    void publish(LogRecord record);

    std::atomic<LogLevel> m_level;
    std::mutex m_mutex;
    std::shared_ptr<const SinkSnapshot> m_sinkSnapshot;
};

[[nodiscard]] Logger& defaultLogger() noexcept;
void setDefaultLogger(Logger* logger) noexcept;
void setDefaultLogWriter(std::nullptr_t) noexcept;

// Sets the process-wide default writer without taking ownership. The caller
// must keep writer alive until the default writer is changed or cleared.
template <typename Writer>
    requires(LogWriter<Writer> || StructuredLogWriter<Writer>)
void setDefaultLogWriter(Writer* writer) noexcept {
    if (writer == nullptr) {
        detail::setDefaultLogWriterRef({});
        return;
    }

    detail::setDefaultLogWriterRef(detail::ErasedLogWriter{
        .object = writer,
        .isEnabledFn = [](const void* object, LogLevel level) noexcept {
            return static_cast<const Writer*>(object)->isEnabled(level);
        },
        .writeFn = detail::logWriteFn<Writer>(),
        .writeStructuredFn = detail::structuredLogWriteFn<Writer>(),
    });
}

// Lightweight context-bound logging proxy. Explicit writers are statically
// dispatched; log(ctx) uses the current default writer reference.
template <LogWriter Writer>
class ContextLogger {
public:
    ContextLogger(std::optional<TraceContext> context, Writer writer, SourceLocation source)
        : m_context(std::move(context)),
          m_writer(std::move(writer)),
          m_source(source) {
    }

    template <typename... Args>
    void trace(std::format_string<Args...> fmt, Args&&... args) {
        write<LogLevel::kTrace>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(std::format_string<Args...> fmt, Args&&... args) {
        write<LogLevel::kDebug>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(std::format_string<Args...> fmt, Args&&... args) {
        write<LogLevel::kInfo>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(std::format_string<Args...> fmt, Args&&... args) {
        write<LogLevel::kWarn>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(std::format_string<Args...> fmt, Args&&... args) {
        write<LogLevel::kError>(fmt, std::forward<Args>(args)...);
    }

private:
    template <LogLevel kLevel, typename... Args>
    void write(std::format_string<Args...> fmt, Args&&... args) {
        if (!m_writer.isEnabled(kLevel)) {
            return;
        }

        m_writer.write(LogRecord{
            .level = kLevel,
            .message = std::format(fmt, std::forward<Args>(args)...),
            .source = m_source,
            .context = m_context,
        });
    }

    std::optional<TraceContext> m_context;
    Writer m_writer;
    SourceLocation m_source;
};

// Lightweight context-bound structured event proxy. Fields are borrowed only
// for the duration of the write call; writers must not retain the field span.
template <StructuredLogWriter Writer>
class ContextEventLogger {
public:
    ContextEventLogger(std::optional<TraceContext> context, Writer writer, SourceLocation source)
        : m_context(std::move(context)),
          m_writer(std::move(writer)),
          m_source(source) {
    }

    template <typename... Fields>
    void trace(std::string_view name, Fields&&... fields) {
        write<LogLevel::kTrace>(name, std::forward<Fields>(fields)...);
    }

    template <typename... Fields>
    void debug(std::string_view name, Fields&&... fields) {
        write<LogLevel::kDebug>(name, std::forward<Fields>(fields)...);
    }

    template <typename... Fields>
    void info(std::string_view name, Fields&&... fields) {
        write<LogLevel::kInfo>(name, std::forward<Fields>(fields)...);
    }

    template <typename... Fields>
    void warn(std::string_view name, Fields&&... fields) {
        write<LogLevel::kWarn>(name, std::forward<Fields>(fields)...);
    }

    template <typename... Fields>
    void error(std::string_view name, Fields&&... fields) {
        write<LogLevel::kError>(name, std::forward<Fields>(fields)...);
    }

private:
    template <LogLevel kLevel, typename... Fields>
    void write(std::string_view name, Fields&&... fields) {
        if (!m_writer.isEnabled(kLevel)) {
            return;
        }

        std::array<LogField, sizeof...(Fields)> fieldArray{std::forward<Fields>(fields)...};
        m_writer.write(StructuredLogRecord{
            .level = kLevel,
            .name = name,
            .fields = std::span<const LogField>(fieldArray.data(), fieldArray.size()),
            .source = m_source,
            .context = m_context,
        });
    }

    std::optional<TraceContext> m_context;
    Writer m_writer;
    SourceLocation m_source;
};

// Creates a context-bound logger backed by the current process-wide writer.
[[nodiscard]] inline ContextLogger<detail::ErasedLogWriter> log(
    std::optional<TraceContext> context,
    SourceLocation source = SourceLocation::current()) {
    return ContextLogger<detail::ErasedLogWriter>(
        std::move(context),
        detail::defaultLogWriterRef(),
        source);
}

// Creates a context-bound logger backed by an explicit writer.
template <LogWriter Writer>
[[nodiscard]] ContextLogger<detail::BorrowedLogWriter<Writer>> log(
    std::optional<TraceContext> context,
    Writer& writer,
    SourceLocation source = SourceLocation::current()) {
    return ContextLogger<detail::BorrowedLogWriter<Writer>>(
        std::move(context),
        detail::BorrowedLogWriter<Writer>(writer),
        source);
}

// Creates a context-bound structured event logger backed by the current writer.
[[nodiscard]] inline ContextEventLogger<detail::ErasedLogWriter> event(
    std::optional<TraceContext> context,
    SourceLocation source = SourceLocation::current()) {
    return ContextEventLogger<detail::ErasedLogWriter>(
        std::move(context),
        detail::defaultLogWriterRef(),
        source);
}

// Creates a context-bound structured event logger backed by an explicit writer.
template <StructuredLogWriter Writer>
[[nodiscard]] ContextEventLogger<detail::BorrowedLogWriter<Writer>> event(
    std::optional<TraceContext> context,
    Writer& writer,
    SourceLocation source = SourceLocation::current()) {
    return ContextEventLogger<detail::BorrowedLogWriter<Writer>>(
        std::move(context),
        detail::BorrowedLogWriter<Writer>(writer),
        source);
}

template <LogLevel kLevel, typename... Args>
void logAt(SourceLocation source, std::format_string<Args...> fmt, Args&&... args) {
    auto writer = detail::defaultLogWriterRef();
    if (!writer.isEnabled(kLevel)) {
        return;
    }

    writer.write(LogRecord{
        .level = kLevel,
        .message = std::format(fmt, std::forward<Args>(args)...),
        .source = source,
        .context = currentContext(),
    });
}

template <LogLevel kLevel, typename... Args>
void logWithContextAt(
    SourceLocation source,
    std::optional<TraceContext> context,
    std::format_string<Args...> fmt,
    Args&&... args) {
    auto writer = detail::defaultLogWriterRef();
    if (!writer.isEnabled(kLevel)) {
        return;
    }

    writer.write(LogRecord{
        .level = kLevel,
        .message = std::format(fmt, std::forward<Args>(args)...),
        .source = source,
        .context = std::move(context),
    });
}

template <typename... Args>
void logTraceAt(SourceLocation source, std::format_string<Args...> fmt, Args&&... args) {
    logAt<LogLevel::kTrace>(source, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logTraceWithContextAt(
    SourceLocation source,
    std::optional<TraceContext> context,
    std::format_string<Args...> fmt,
    Args&&... args) {
    logWithContextAt<LogLevel::kTrace>(source, std::move(context), fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logDebugAt(SourceLocation source, std::format_string<Args...> fmt, Args&&... args) {
    logAt<LogLevel::kDebug>(source, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logDebugWithContextAt(
    SourceLocation source,
    std::optional<TraceContext> context,
    std::format_string<Args...> fmt,
    Args&&... args) {
    logWithContextAt<LogLevel::kDebug>(source, std::move(context), fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logInfoAt(SourceLocation source, std::format_string<Args...> fmt, Args&&... args) {
    logAt<LogLevel::kInfo>(source, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logInfoWithContextAt(
    SourceLocation source,
    std::optional<TraceContext> context,
    std::format_string<Args...> fmt,
    Args&&... args) {
    logWithContextAt<LogLevel::kInfo>(source, std::move(context), fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logWarnAt(SourceLocation source, std::format_string<Args...> fmt, Args&&... args) {
    logAt<LogLevel::kWarn>(source, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logWarnWithContextAt(
    SourceLocation source,
    std::optional<TraceContext> context,
    std::format_string<Args...> fmt,
    Args&&... args) {
    logWithContextAt<LogLevel::kWarn>(source, std::move(context), fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logErrorAt(SourceLocation source, std::format_string<Args...> fmt, Args&&... args) {
    logAt<LogLevel::kError>(source, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logErrorWithContextAt(
    SourceLocation source,
    std::optional<TraceContext> context,
    std::format_string<Args...> fmt,
    Args&&... args) {
    logWithContextAt<LogLevel::kError>(source, std::move(context), fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logTrace(std::format_string<Args...> fmt, Args&&... args) {
    logTraceAt(SourceLocation::current(), fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logDebug(std::format_string<Args...> fmt, Args&&... args) {
    logDebugAt(SourceLocation::current(), fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logInfo(std::format_string<Args...> fmt, Args&&... args) {
    logInfoAt(SourceLocation::current(), fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logWarn(std::format_string<Args...> fmt, Args&&... args) {
    logWarnAt(SourceLocation::current(), fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logError(std::format_string<Args...> fmt, Args&&... args) {
    logErrorAt(SourceLocation::current(), fmt, std::forward<Args>(args)...);
}

} // namespace galay::tracing

#define GALAY_LOG_TRACE(...) \
    ::galay::tracing::logTraceAt(::galay::tracing::SourceLocation{__FILE__, __LINE__, __func__}, __VA_ARGS__)
#define GALAY_LOG_TRACE_CTX(context, ...) \
    ::galay::tracing::logTraceWithContextAt(::galay::tracing::SourceLocation{__FILE__, __LINE__, __func__}, (context), __VA_ARGS__)
#define GALAY_LOG_DEBUG(...) \
    ::galay::tracing::logDebugAt(::galay::tracing::SourceLocation{__FILE__, __LINE__, __func__}, __VA_ARGS__)
#define GALAY_LOG_DEBUG_CTX(context, ...) \
    ::galay::tracing::logDebugWithContextAt(::galay::tracing::SourceLocation{__FILE__, __LINE__, __func__}, (context), __VA_ARGS__)
#define GALAY_LOG_INFO(...) \
    ::galay::tracing::logInfoAt(::galay::tracing::SourceLocation{__FILE__, __LINE__, __func__}, __VA_ARGS__)
#define GALAY_LOG_INFO_CTX(context, ...) \
    ::galay::tracing::logInfoWithContextAt(::galay::tracing::SourceLocation{__FILE__, __LINE__, __func__}, (context), __VA_ARGS__)
#define GALAY_LOG_WARN(...) \
    ::galay::tracing::logWarnAt(::galay::tracing::SourceLocation{__FILE__, __LINE__, __func__}, __VA_ARGS__)
#define GALAY_LOG_WARN_CTX(context, ...) \
    ::galay::tracing::logWarnWithContextAt(::galay::tracing::SourceLocation{__FILE__, __LINE__, __func__}, (context), __VA_ARGS__)
#define GALAY_LOG_ERROR(...) \
    ::galay::tracing::logErrorAt(::galay::tracing::SourceLocation{__FILE__, __LINE__, __func__}, __VA_ARGS__)
#define GALAY_LOG_ERROR_CTX(context, ...) \
    ::galay::tracing::logErrorWithContextAt(::galay::tracing::SourceLocation{__FILE__, __LINE__, __func__}, (context), __VA_ARGS__)

// Rust-like structured event callsite. The level check happens before context
// and field expressions are evaluated, so disabled events avoid field creation.
#define GALAY_EVENT_AT(level, writer, context, name, ...)                                                   \
    do {                                                                                                    \
        auto&& galayTracingWriter = (writer);                                                               \
        constexpr auto galayTracingLevel = (level);                                                         \
        if (galayTracingWriter.isEnabled(galayTracingLevel)) {                                              \
            ::galay::tracing::detail::writeEventUnchecked<galayTracingLevel>(                               \
                galayTracingWriter,                                                                         \
                (context),                                                                                  \
                ::galay::tracing::SourceLocation{__FILE__, __LINE__, __func__},                             \
                (name) __VA_OPT__(, ) __VA_ARGS__);                                                         \
        }                                                                                                   \
    } while (false)

#define GALAY_EVENT_TRACE(writer, context, name, ...) \
    GALAY_EVENT_AT(::galay::tracing::LogLevel::kTrace, writer, context, name __VA_OPT__(, ) __VA_ARGS__)
#define GALAY_EVENT_DEBUG(writer, context, name, ...) \
    GALAY_EVENT_AT(::galay::tracing::LogLevel::kDebug, writer, context, name __VA_OPT__(, ) __VA_ARGS__)
#define GALAY_EVENT_INFO(writer, context, name, ...) \
    GALAY_EVENT_AT(::galay::tracing::LogLevel::kInfo, writer, context, name __VA_OPT__(, ) __VA_ARGS__)
#define GALAY_EVENT_WARN(writer, context, name, ...) \
    GALAY_EVENT_AT(::galay::tracing::LogLevel::kWarn, writer, context, name __VA_OPT__(, ) __VA_ARGS__)
#define GALAY_EVENT_ERROR(writer, context, name, ...) \
    GALAY_EVENT_AT(::galay::tracing::LogLevel::kError, writer, context, name __VA_OPT__(, ) __VA_ARGS__)
