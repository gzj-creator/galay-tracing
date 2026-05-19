#pragma once

#include "galay-tracing/kernel/span.h"

#include <chrono>
#include <span>

namespace galay::tracing {

enum class ExportResult {
    kSuccess,
    kFailure,
};

class SpanExporter {
public:
    virtual ~SpanExporter() = default;

    virtual ExportResult exportSpans(std::span<const Span> spans) = 0;

    virtual bool forceFlush(std::chrono::milliseconds timeout);
    virtual bool shutdown(std::chrono::milliseconds timeout);
};

} // namespace galay::tracing
