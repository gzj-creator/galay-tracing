#include "galay-tracing/kernel/sampler.h"

namespace galay::tracing {

bool AlwaysOnSampler::shouldSample(const TraceContext&) const noexcept {
    return true;
}

bool AlwaysOffSampler::shouldSample(const TraceContext&) const noexcept {
    return false;
}

} // namespace galay::tracing
