#pragma once

#include "galay-tracing/common/source_location.h"
#include "galay-tracing/context/context_storage.h"
#include "galay-tracing/log/log_level.h"
#include "galay-tracing/log/log_record.h"
#include "galay-tracing/log/log_sink.h"

#include <atomic>
#include <format>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace galay::tracing {

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

private:
    void publish(LogRecord record);

    std::atomic<LogLevel> m_level;
    std::mutex m_mutex;
    std::vector<std::shared_ptr<LogSink>> m_sinks;
};

[[nodiscard]] Logger& defaultLogger() noexcept;
void setDefaultLogger(Logger* logger) noexcept;

template <typename... Args>
void logTraceAt(SourceLocation source, std::format_string<Args...> fmt, Args&&... args) {
    defaultLogger().log(LogLevel::kTrace, source, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logDebugAt(SourceLocation source, std::format_string<Args...> fmt, Args&&... args) {
    defaultLogger().log(LogLevel::kDebug, source, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logInfoAt(SourceLocation source, std::format_string<Args...> fmt, Args&&... args) {
    defaultLogger().log(LogLevel::kInfo, source, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logWarnAt(SourceLocation source, std::format_string<Args...> fmt, Args&&... args) {
    defaultLogger().log(LogLevel::kWarn, source, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logErrorAt(SourceLocation source, std::format_string<Args...> fmt, Args&&... args) {
    defaultLogger().log(LogLevel::kError, source, fmt, std::forward<Args>(args)...);
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
#define GALAY_LOG_DEBUG(...) \
    ::galay::tracing::logDebugAt(::galay::tracing::SourceLocation{__FILE__, __LINE__, __func__}, __VA_ARGS__)
#define GALAY_LOG_INFO(...) \
    ::galay::tracing::logInfoAt(::galay::tracing::SourceLocation{__FILE__, __LINE__, __func__}, __VA_ARGS__)
#define GALAY_LOG_WARN(...) \
    ::galay::tracing::logWarnAt(::galay::tracing::SourceLocation{__FILE__, __LINE__, __func__}, __VA_ARGS__)
#define GALAY_LOG_ERROR(...) \
    ::galay::tracing::logErrorAt(::galay::tracing::SourceLocation{__FILE__, __LINE__, __func__}, __VA_ARGS__)
