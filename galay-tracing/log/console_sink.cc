#include "galay-tracing/log/console_sink.h"

#include "galay-tracing/log/log_level.h"

#include <iostream>

namespace galay::tracing {

ConsoleSink::ConsoleSink(std::ostream& out)
    : m_out(&out) {
}

ConsoleSink::ConsoleSink()
    : ConsoleSink(std::cout) {
}

void ConsoleSink::write(const LogRecord& record) {
    auto& out = *m_out;
    out << "level=" << logLevelName(record.level);
    if (record.context.has_value()) {
        out << " trace_id=" << record.context->traceId().toHex() << " span_id=" << record.context->spanId().toHex();
    }
    out << " file=" << record.source.file << ':' << record.source.line << " msg=\"" << record.message << "\"\n";
}

} // namespace galay::tracing
