#pragma once

#include "galay-tracing/log/log_record.h"

namespace galay::tracing {

class LogSink {
public:
    virtual ~LogSink() = default;

    virtual void write(const LogRecord& record) = 0;
};

} // namespace galay::tracing
