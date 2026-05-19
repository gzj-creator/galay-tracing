#include "galay-tracing/kernel/span.h"

#include <utility>

namespace galay::tracing {

Span::Span(std::string name, TraceContext context)
    : m_name(std::move(name)),
      m_context(std::move(context)),
      m_startedAt(Clock::now()) {
}

void Span::end() noexcept {
    if (!m_ended) {
        m_endedAt = Clock::now();
        m_ended = true;
    }
}

} // namespace galay::tracing
