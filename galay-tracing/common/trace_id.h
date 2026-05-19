#pragma once

#include "galay-tracing/common/id_format.h"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace galay::tracing {

class TraceId {
public:
    static constexpr std::size_t kByteLength = 16;
    static constexpr std::size_t kHexLength = kByteLength * 2;

    using Bytes = std::array<std::byte, kByteLength>;

    constexpr TraceId() noexcept = default;

    [[nodiscard]] static constexpr TraceId zero() noexcept {
        return TraceId{};
    }

    [[nodiscard]] static TraceId random() noexcept;

    // Parses exactly 32 hex characters. Invalid, malformed, or all-zero input returns zero().
    [[nodiscard]] static TraceId fromHex(std::string_view hex) noexcept {
        TraceId id;
        if (!detail::parseHex(hex, id.m_bytes.data(), id.m_bytes.size()) || !id.isValid()) {
            return zero();
        }
        return id;
    }

    [[nodiscard]] constexpr bool isValid() const noexcept {
        return detail::hasNonZeroByte(m_bytes.data(), m_bytes.size());
    }

    [[nodiscard]] std::string toHex() const {
        std::string hex(kHexLength, '\0');
        static_cast<void>(toHex(hex.data(), hex.size()));
        return hex;
    }

    [[nodiscard]] bool toHex(char* out, std::size_t len) const noexcept {
        return detail::formatHex(m_bytes.data(), m_bytes.size(), out, len);
    }

    [[nodiscard]] std::array<char, kHexLength> toHexArray() const noexcept {
        std::array<char, kHexLength> hex{};
        static_cast<void>(toHex(hex.data(), hex.size()));
        return hex;
    }

    [[nodiscard]] constexpr const Bytes& bytes() const noexcept {
        return m_bytes;
    }

    friend constexpr bool operator==(const TraceId&, const TraceId&) noexcept = default;

private:
    friend TraceId detail::makeRandomTraceId() noexcept;

    explicit constexpr TraceId(Bytes bytes) noexcept
        : m_bytes(bytes) {
    }

    Bytes m_bytes{};
};

} // namespace galay::tracing
