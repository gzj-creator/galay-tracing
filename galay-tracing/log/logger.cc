#include "galay-tracing/log/logger.h"

#include "galay-tracing/log/console_sink.h"

#include <algorithm>
#include <format>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace galay::tracing {

namespace {

std::atomic<Logger*> g_defaultLogger{nullptr};

struct DefaultLogWriterSnapshot {
    detail::ErasedLogWriter writer;
};

std::mutex g_defaultWriterConfigMutex;
std::vector<std::unique_ptr<DefaultLogWriterSnapshot>> g_defaultWriterSnapshots;
std::atomic<const DefaultLogWriterSnapshot*> g_defaultWriterSnapshot{nullptr};

[[nodiscard]] detail::ErasedLogWriter loggerWriterRef(Logger& logger) noexcept {
    return detail::ErasedLogWriter{
        .object = &logger,
        .isEnabledFn = [](const void* object, LogLevel level) noexcept {
            return static_cast<const Logger*>(object)->isEnabled(level);
        },
        .writeFn = [](void* object, LogRecord record) {
            static_cast<Logger*>(object)->write(std::move(record));
        },
        .writeStructuredFn = [](void* object, StructuredLogRecord record) {
            static_cast<Logger*>(object)->write(record);
        },
    };
}

[[nodiscard]] Logger& builtInDefaultLogger() {
    static Logger logger;
    static const bool configured = [] {
        logger.addSink(std::make_shared<ConsoleSink>());
        return true;
    }();
    (void)configured;
    return logger;
}

void appendFieldValue(std::string& message, const LogFieldValue& value) {
    switch (value.type) {
    case LogFieldType::kInt64:
        message.append(std::to_string(value.int64_value));
        break;
    case LogFieldType::kUInt64:
        message.append(std::to_string(value.uint64_value));
        break;
    case LogFieldType::kDouble:
        message.append(std::to_string(value.double_value));
        break;
    case LogFieldType::kBool:
        message.append(value.bool_value ? "true" : "false");
        break;
    case LogFieldType::kString:
        message.append(value.string_value);
        break;
    }
}

[[nodiscard]] LogRecord makeLogRecord(StructuredLogRecord record) {
    std::string message(record.name);
    for (const auto& field : record.fields) {
        message.push_back(' ');
        message.append(field.name);
        message.push_back('=');
        appendFieldValue(message, field.value);
    }

    return LogRecord{
        .level = record.level,
        .message = std::move(message),
        .source = record.source,
        .context = std::move(record.context),
    };
}

} // namespace

namespace detail {

void ErasedLogWriter::write(StructuredLogRecord record) {
    if (object == nullptr) {
        return;
    }
    if (writeStructuredFn != nullptr) {
        writeStructuredFn(object, record);
        return;
    }
    if (writeFn != nullptr) {
        writeFn(object, makeLogRecord(record));
    }
}

void setDefaultLogWriterRef(ErasedLogWriter writer) noexcept {
    if (writer.object == nullptr) {
        g_defaultWriterSnapshot.store(nullptr, std::memory_order_release);
        return;
    }

    auto next = std::make_unique<DefaultLogWriterSnapshot>(writer);
    auto* snapshot = next.get();
    {
        std::lock_guard lock(g_defaultWriterConfigMutex);
        // Keep old snapshots alive for lock-free readers that already loaded
        // the previous pointer. Default writer configuration is rare.
        g_defaultWriterSnapshots.push_back(std::move(next));
    }
    g_defaultWriterSnapshot.store(snapshot, std::memory_order_release);
}

ErasedLogWriter defaultLogWriterRef() noexcept {
    if (auto* snapshot = g_defaultWriterSnapshot.load(std::memory_order_acquire);
        snapshot && snapshot->writer.object != nullptr) {
        return snapshot->writer;
    }

    return loggerWriterRef(defaultLogger());
}

} // namespace detail

Logger::Logger(LogLevel level) noexcept
    : m_level(level),
      m_sinkSnapshot(std::make_shared<SinkSnapshot>()) {
}

void Logger::setLevel(LogLevel level) noexcept {
    m_level.store(level, std::memory_order_relaxed);
}

LogLevel Logger::level() const noexcept {
    return m_level.load(std::memory_order_relaxed);
}

bool Logger::isEnabled(LogLevel recordLevel) const noexcept {
    const auto threshold = level();
    return threshold != LogLevel::kOff && static_cast<int>(recordLevel) >= static_cast<int>(threshold);
}

void Logger::addSink(std::shared_ptr<LogSink> sink) {
    if (!sink) {
        return;
    }

    std::lock_guard lock(m_mutex);
    auto next = std::make_shared<SinkSnapshot>();
    if (auto current = std::atomic_load_explicit(&m_sinkSnapshot, std::memory_order_acquire); current) {
        next->sinks = current->sinks;
    }
    next->sinks.push_back(std::move(sink));
    std::atomic_store_explicit(&m_sinkSnapshot, std::shared_ptr<const SinkSnapshot>(std::move(next)), std::memory_order_release);
}

void Logger::clearSinks() {
    std::lock_guard lock(m_mutex);
    std::atomic_store_explicit(&m_sinkSnapshot, std::shared_ptr<const SinkSnapshot>(std::make_shared<SinkSnapshot>()), std::memory_order_release);
}

void Logger::write(LogRecord record) {
    publish(std::move(record));
}

void Logger::write(StructuredLogRecord record) {
    publish(makeLogRecord(record));
}

void Logger::publish(LogRecord record) {
    const auto snapshot = std::atomic_load_explicit(&m_sinkSnapshot, std::memory_order_acquire);
    if (!snapshot) {
        return;
    }

    for (const auto& sink : snapshot->sinks) {
        sink->write(record);
    }
}

Logger& defaultLogger() noexcept {
    if (auto* logger = g_defaultLogger.load(std::memory_order_acquire); logger != nullptr) {
        return *logger;
    }
    return builtInDefaultLogger();
}

void setDefaultLogger(Logger* logger) noexcept {
    g_defaultLogger.store(logger, std::memory_order_release);
    setDefaultLogWriter(logger);
}

void setDefaultLogWriter(std::nullptr_t) noexcept {
    detail::setDefaultLogWriterRef({});
}

} // namespace galay::tracing
