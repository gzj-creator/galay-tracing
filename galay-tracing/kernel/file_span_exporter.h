#pragma once

#include "galay-tracing/kernel/span_exporter.h"

#include <filesystem>
#include <fstream>
#include <mutex>

namespace galay::tracing {

class FileSpanExporter final : public SpanExporter {
public:
    explicit FileSpanExporter(const std::filesystem::path& path);

    ExportResult exportSpans(std::span<const Span> spans) override;
    bool forceFlush(std::chrono::milliseconds timeout) override;
    bool shutdown(std::chrono::milliseconds timeout) override;

private:
    std::mutex m_mutex;
    std::ofstream m_out;
};

} // namespace galay::tracing
