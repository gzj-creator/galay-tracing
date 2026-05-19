#include "galay-tracing/log/logger.h"

#include "galay-tracing/log/console_sink.h"

#include <algorithm>

namespace galay::tracing {

namespace {

std::atomic<Logger*> g_defaultLogger{nullptr};

[[nodiscard]] Logger& builtInDefaultLogger() {
    static Logger logger;
    static const bool configured = [] {
        logger.addSink(std::make_shared<ConsoleSink>());
        return true;
    }();
    (void)configured;
    return logger;
}

} // namespace

Logger::Logger(LogLevel level) noexcept
    : m_level(level) {
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
    m_sinks.push_back(std::move(sink));
}

void Logger::clearSinks() {
    std::lock_guard lock(m_mutex);
    m_sinks.clear();
}

void Logger::publish(LogRecord record) {
    std::vector<std::shared_ptr<LogSink>> sinks;
    {
        std::lock_guard lock(m_mutex);
        sinks = m_sinks;
    }

    for (const auto& sink : sinks) {
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
}

} // namespace galay::tracing
