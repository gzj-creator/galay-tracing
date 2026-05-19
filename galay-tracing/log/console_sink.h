#pragma once

#include "galay-tracing/log/log_sink.h"

#include <iosfwd>

namespace galay::tracing {

class ConsoleSink final : public LogSink {
public:
    explicit ConsoleSink(std::ostream& out);
    ConsoleSink();

    void write(const LogRecord& record) override;

private:
    std::ostream* m_out;
};

} // namespace galay::tracing
