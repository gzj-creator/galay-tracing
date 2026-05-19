#include "galay-tracing/kernel/sampler.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace galay::tracing {

namespace {

std::atomic<const Sampler*> g_sampler{nullptr};

[[nodiscard]] std::uint64_t traceIdHighBits(const TraceId& traceId) noexcept {
    std::uint64_t value = 0;
    const auto& bytes = traceId.bytes();
    for (std::size_t i = 0; i < sizeof(std::uint64_t); ++i) {
        value = (value << 8U) | static_cast<std::uint64_t>(static_cast<unsigned char>(bytes[i]));
    }
    return value;
}

[[nodiscard]] const Sampler& builtInSampler() noexcept {
    static const AlwaysOnSampler rootSampler;
    static const ParentBasedSampler sampler(rootSampler);
    return sampler;
}

} // namespace

bool AlwaysOnSampler::shouldSample(const SpanContext*, const TraceId&) const noexcept {
    return true;
}

bool AlwaysOffSampler::shouldSample(const SpanContext*, const TraceId&) const noexcept {
    return false;
}

ParentBasedSampler::ParentBasedSampler(const Sampler& rootSampler) noexcept
    : m_rootSampler(&rootSampler) {
}

bool ParentBasedSampler::shouldSample(const SpanContext* parent, const TraceId& traceId) const noexcept {
    if (parent != nullptr && parent->isValid()) {
        return parent->sampled();
    }
    return m_rootSampler == nullptr || m_rootSampler->shouldSample(nullptr, traceId);
}

TraceIdRatioSampler::TraceIdRatioSampler(double ratio) noexcept
    : m_ratio(std::clamp(ratio, 0.0, 1.0)) {
}

bool TraceIdRatioSampler::shouldSample(const SpanContext*, const TraceId& traceId) const noexcept {
    if (m_ratio <= 0.0) {
        return false;
    }
    if (m_ratio >= 1.0) {
        return true;
    }

    constexpr long double kDenominator = static_cast<long double>(UINT64_MAX) + 1.0L;
    const auto normalized = static_cast<long double>(traceIdHighBits(traceId)) / kDenominator;
    return normalized < static_cast<long double>(m_ratio);
}

void setSampler(const Sampler* sampler) noexcept {
    g_sampler.store(sampler, std::memory_order_release);
}

const Sampler& currentSampler() noexcept {
    if (auto* sampler = g_sampler.load(std::memory_order_acquire); sampler != nullptr) {
        return *sampler;
    }
    return builtInSampler();
}

} // namespace galay::tracing
