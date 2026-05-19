#pragma once

#include "galay-tracing/kernel/span.h"

#include <chrono>

namespace galay::tracing {

class SpanProcessor {
public:
    virtual ~SpanProcessor() = default;

    virtual void onEnd(Span span) = 0;
    virtual bool forceFlush(std::chrono::milliseconds timeout) = 0;
    virtual bool shutdown(std::chrono::milliseconds timeout) = 0;
};

} // namespace galay::tracing
