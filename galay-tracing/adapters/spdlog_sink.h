#pragma once

#include "galay-tracing/log/log_sink.h"

#include <memory>

namespace spdlog {
class logger;
} // namespace spdlog

namespace galay::tracing {

class SpdlogSink final : public LogSink {
public:
    explicit SpdlogSink(std::shared_ptr<spdlog::logger> logger);

    void write(const LogRecord& record) override;

private:
    std::shared_ptr<spdlog::logger> m_logger;
};

} // namespace galay::tracing
