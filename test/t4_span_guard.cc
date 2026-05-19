#include "galay-tracing/context/context_storage.h"
#include "galay-tracing/kernel/span_guard.h"

#include <cassert>
#include <type_traits>
#include <utility>

namespace {

void rootSpanCreatesAndRestoresContext() {
    galay::tracing::clearCurrentContext();
    galay::tracing::TraceId traceId;
    galay::tracing::SpanId spanId;

    {
        auto guard = galay::tracing::startSpan("root");
        auto current = galay::tracing::currentContext();

        assert(current.has_value());
        assert(current->traceId().isValid());
        assert(current->spanId().isValid());
        assert(!current->parentSpanId().has_value());
        assert(guard.span().name() == "root");

        traceId = current->traceId();
        spanId = current->spanId();
    }

    assert(traceId.isValid());
    assert(spanId.isValid());
    assert(!galay::tracing::currentContext().has_value());
}

void nestedSpanUsesParentTraceAndRestoresParent() {
    galay::tracing::clearCurrentContext();

    auto root = galay::tracing::startSpan("root");
    const auto rootContext = galay::tracing::currentContext();
    assert(rootContext.has_value());

    {
        auto child = galay::tracing::startSpan("child");
        const auto childContext = galay::tracing::currentContext();

        assert(childContext.has_value());
        assert(childContext->traceId() == rootContext->traceId());
        assert(childContext->spanId() != rootContext->spanId());
        assert(childContext->parentSpanId().has_value());
        assert(*childContext->parentSpanId() == rootContext->spanId());
        assert(child.span().name() == "child");
    }

    assert(galay::tracing::currentContext() == rootContext);
}

void startServerSpanUsesInboundParentAndRestoresPrevious() {
    auto inbound = galay::tracing::TraceContext(
        galay::tracing::TraceId::fromHex("4bf92f3577b34da6a3ce929d0e0e4736"),
        galay::tracing::SpanId::fromHex("00f067aa0ba902b7"),
        0x01,
        "vendor=value");
    auto previous = galay::tracing::TraceContext(
        galay::tracing::TraceId::fromHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),
        galay::tracing::SpanId::fromHex("bbbbbbbbbbbbbbbb"));

    galay::tracing::setCurrentContext(previous);

    {
        auto server = galay::tracing::startServerSpan("server", inbound);
        const auto current = galay::tracing::currentContext();

        assert(current.has_value());
        assert(current->traceId() == inbound.traceId());
        assert(current->spanId() != inbound.spanId());
        assert(current->parentSpanId().has_value());
        assert(*current->parentSpanId() == inbound.spanId());
        assert(current->sampled());
        assert(current->tracestate() == inbound.tracestate());
        assert(server.span().name() == "server");
    }

    assert(galay::tracing::currentContext() == previous);
    galay::tracing::clearCurrentContext();
}

void spanGuardIsMoveOnlyAndRestoresOnce() {
    static_assert(!std::is_copy_constructible_v<galay::tracing::SpanGuard>);
    static_assert(!std::is_copy_assignable_v<galay::tracing::SpanGuard>);
    static_assert(std::is_move_constructible_v<galay::tracing::SpanGuard>);
    static_assert(noexcept(std::declval<galay::tracing::SpanGuard&>().~SpanGuard()));

    galay::tracing::clearCurrentContext();

    {
        galay::tracing::SpanGuard moved;
        {
            auto guard = galay::tracing::startSpan("moved");
            const auto active = galay::tracing::currentContext();
            moved = std::move(guard);
            assert(galay::tracing::currentContext() == active);
        }

        assert(galay::tracing::currentContext().has_value());
    }

    assert(!galay::tracing::currentContext().has_value());
}

} // namespace

int main() {
    rootSpanCreatesAndRestoresContext();
    nestedSpanUsesParentTraceAndRestoresParent();
    startServerSpanUsesInboundParentAndRestoresPrevious();
    spanGuardIsMoveOnlyAndRestoresOnce();
}
