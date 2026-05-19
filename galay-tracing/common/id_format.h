#pragma once

#include <cstddef>
#include <string_view>

namespace galay::tracing {

class SpanId;
class TraceId;

namespace detail {

[[nodiscard]] constexpr int hexValue(char ch) noexcept {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    return -1;
}

[[nodiscard]] constexpr bool hasNonZeroByte(const std::byte* bytes, std::size_t len) noexcept {
    for (std::size_t i = 0; i < len; ++i) {
        if (bytes[i] != std::byte{0}) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool parseHex(std::string_view hex, std::byte* out, std::size_t len) noexcept;
[[nodiscard]] bool formatHex(const std::byte* bytes, std::size_t byteLen, char* out, std::size_t outLen) noexcept;

[[nodiscard]] TraceId makeRandomTraceId() noexcept;
[[nodiscard]] SpanId makeRandomSpanId() noexcept;

} // namespace detail
} // namespace galay::tracing
