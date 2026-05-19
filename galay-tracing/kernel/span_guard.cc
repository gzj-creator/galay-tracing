#include "galay-tracing/kernel/span_guard.h"

#include "galay-tracing/context/context_storage.h"

#include <string>
#include <utility>

namespace galay::tracing {

namespace {

[[nodiscard]] TraceContext makeRootContext() {
    return TraceContext(TraceId::random(), SpanId::random());
}

[[nodiscard]] TraceContext makeChildContext(const TraceContext& parent) {
    TraceContext context(parent.traceId(), SpanId::random(), parent.traceFlags(), parent.tracestate());
    context.setParentSpanId(parent.spanId());
    return context;
}

[[nodiscard]] TraceContext makeNextContext(const std::optional<TraceContext>& parent) {
    if (parent.has_value() && parent->isValid()) {
        return makeChildContext(*parent);
    }
    return makeRootContext();
}

} // namespace

SpanGuard::SpanGuard(Span span, std::optional<TraceContext> previousContext)
    : m_span(std::move(span)),
      m_previousContext(std::move(previousContext)),
      m_active(true) {
}

SpanGuard::~SpanGuard() noexcept {
    restore();
}

SpanGuard::SpanGuard(SpanGuard&& other) noexcept
    : m_span(std::move(other.m_span)),
      m_previousContext(std::move(other.m_previousContext)),
      m_active(other.m_active) {
    other.m_active = false;
}

SpanGuard& SpanGuard::operator=(SpanGuard&& other) noexcept {
    if (this != &other) {
        restore();
        m_span = std::move(other.m_span);
        m_previousContext = std::move(other.m_previousContext);
        m_active = other.m_active;
        other.m_active = false;
    }
    return *this;
}

void SpanGuard::end() noexcept {
    if (m_active) {
        m_span.end();
    }
}

void SpanGuard::restore() noexcept {
    if (!m_active) {
        return;
    }

    end();
    try {
        setCurrentContext(std::move(m_previousContext));
    } catch (...) {
        clearCurrentContext();
    }
    m_active = false;
}

SpanGuard startSpan(std::string_view name) {
    auto previous = currentContext();
    auto context = makeNextContext(previous);
    Span span(std::string(name), context);
    setCurrentContext(context);
    return SpanGuard(std::move(span), std::move(previous));
}

SpanGuard startServerSpan(std::string_view name, const TraceContext& parent) {
    auto previous = currentContext();
    auto context = parent.isValid() ? makeChildContext(parent) : makeRootContext();
    Span span(std::string(name), context);
    setCurrentContext(context);
    return SpanGuard(std::move(span), std::move(previous));
}

} // namespace galay::tracing
