#include "galay-tracing/kernel/file_span_exporter.h"

#include <string>

namespace galay::tracing {

namespace {

void appendJsonString(std::string& out, std::string_view value) {
    out.push_back('"');
    for (const char ch : value) {
        if (ch == '"' || ch == '\\') {
            out.push_back('\\');
        }
        out.push_back(ch);
    }
    out.push_back('"');
}

[[nodiscard]] std::string renderSpanJson(const Span& span) {
    const auto& context = span.spanContext();
    std::string line;
    line.append("{\"name\":");
    appendJsonString(line, span.name());
    line.append(",\"trace_id\":\"");
    line.append(context.traceId().toHex());
    line.append("\",\"span_id\":\"");
    line.append(context.spanId().toHex());
    line.append("\",\"sampled\":");
    line.append(context.sampled() ? "true" : "false");
    line.push_back('}');
    return line;
}

} // namespace

FileSpanExporter::FileSpanExporter(const std::filesystem::path& path)
    : m_out(path, std::ios::out | std::ios::app) {
}

ExportResult FileSpanExporter::exportSpans(std::span<const Span> spans) {
    std::lock_guard lock(m_mutex);
    if (!m_out) {
        return ExportResult::kFailure;
    }

    for (const auto& span : spans) {
        m_out << renderSpanJson(span) << '\n';
    }
    return m_out ? ExportResult::kSuccess : ExportResult::kFailure;
}

bool FileSpanExporter::forceFlush(std::chrono::milliseconds) {
    std::lock_guard lock(m_mutex);
    m_out.flush();
    return static_cast<bool>(m_out);
}

bool FileSpanExporter::shutdown(std::chrono::milliseconds timeout) {
    static_cast<void>(timeout);
    std::lock_guard lock(m_mutex);
    m_out.flush();
    m_out.close();
    return true;
}

} // namespace galay::tracing
