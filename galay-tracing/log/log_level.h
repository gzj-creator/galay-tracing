#pragma once

#include <string_view>

namespace galay::tracing {

enum class LogLevel {
    kTrace = 0,
    kDebug = 1,
    kInfo = 2,
    kWarn = 3,
    kError = 4,
    kOff = 5,
};

[[nodiscard]] constexpr std::string_view logLevelName(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::kTrace:
        return "TRACE";
    case LogLevel::kDebug:
        return "DEBUG";
    case LogLevel::kInfo:
        return "INFO";
    case LogLevel::kWarn:
        return "WARN";
    case LogLevel::kError:
        return "ERROR";
    case LogLevel::kOff:
        return "OFF";
    }
    return "UNKNOWN";
}

} // namespace galay::tracing
