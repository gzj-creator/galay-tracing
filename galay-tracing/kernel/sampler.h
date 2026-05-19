#pragma once

#include "galay-tracing/context/trace_context.h"

namespace galay::tracing {

class Sampler {
public:
    virtual ~Sampler() = default;

    [[nodiscard]] virtual bool shouldSample(const TraceContext& parent) const noexcept = 0;
};

class AlwaysOnSampler final : public Sampler {
public:
    [[nodiscard]] bool shouldSample(const TraceContext&) const noexcept override;
};

class AlwaysOffSampler final : public Sampler {
public:
    [[nodiscard]] bool shouldSample(const TraceContext&) const noexcept override;
};

} // namespace galay::tracing
