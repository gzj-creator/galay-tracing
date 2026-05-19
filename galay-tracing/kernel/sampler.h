#pragma once

#include "galay-tracing/context/trace_context.h"

#include <cstdint>

namespace galay::tracing {

class Sampler {
public:
    virtual ~Sampler() = default;

    [[nodiscard]] virtual bool shouldSample(const SpanContext* parent, const TraceId& traceId) const noexcept = 0;
};

class AlwaysOnSampler final : public Sampler {
public:
    [[nodiscard]] bool shouldSample(const SpanContext* parent, const TraceId& traceId) const noexcept override;
};

class AlwaysOffSampler final : public Sampler {
public:
    [[nodiscard]] bool shouldSample(const SpanContext* parent, const TraceId& traceId) const noexcept override;
};

class ParentBasedSampler final : public Sampler {
public:
    explicit ParentBasedSampler(const Sampler& rootSampler) noexcept;

    [[nodiscard]] bool shouldSample(const SpanContext* parent, const TraceId& traceId) const noexcept override;

private:
    const Sampler* m_rootSampler;
};

class TraceIdRatioSampler final : public Sampler {
public:
    explicit TraceIdRatioSampler(double ratio) noexcept;

    [[nodiscard]] double ratio() const noexcept {
        return m_ratio;
    }

    [[nodiscard]] bool shouldSample(const SpanContext* parent, const TraceId& traceId) const noexcept override;

private:
    double m_ratio{1.0};
};

// Sets the process-wide sampler without taking ownership. Passing nullptr
// restores the built-in parent-based sampler with an always-on root decision.
void setSampler(const Sampler* sampler) noexcept;
[[nodiscard]] const Sampler& currentSampler() noexcept;

} // namespace galay::tracing
