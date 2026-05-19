#pragma once

#include <cstdint>
#include <source_location>

namespace galay::tracing {

struct SourceLocation {
    const char* file{""};
    std::uint_least32_t line{0};
    const char* function{""};

    [[nodiscard]] static consteval SourceLocation current(
        const std::source_location location = std::source_location::current()) noexcept {
        return SourceLocation{location.file_name(), location.line(), location.function_name()};
    }
};

} // namespace galay::tracing
