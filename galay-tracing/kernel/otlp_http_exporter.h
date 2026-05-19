#pragma once

#include "galay-tracing/kernel/span_exporter.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace galay::tracing {

struct OtlpHttpHeader {
    std::string name;
    std::string value;
};

struct InstrumentationScopeConfig {
    std::string name{"galay-tracing"};
    std::string version;
};

// Configuration for OTLP/HTTP JSON trace export.
struct OtlpHttpExporterConfig {
    std::string endpoint{"http://127.0.0.1:4318/v1/traces"};
    std::chrono::milliseconds timeout{std::chrono::milliseconds(500)};
    std::vector<OtlpHttpHeader> headers;
    std::vector<SpanAttribute> resource_attributes;
    InstrumentationScopeConfig scope;
};

struct OtlpHttpRequest {
    // Borrowed views are valid only for the duration of the transport call.
    std::string_view method{"POST"};
    std::string_view endpoint;
    std::chrono::milliseconds timeout{std::chrono::milliseconds(500)};
    std::span<const OtlpHttpHeader> headers;
    std::string body;
};

struct OtlpHttpResponse {
    int status_code{0};
    std::string body;
    std::string error;
};

using OtlpHttpTransport = std::function<OtlpHttpResponse(OtlpHttpRequest request)>;

// Runtime settings for the galay-http backed transport.
struct GalayHttpOtlpTransportConfig {
    std::size_t io_scheduler_count{1};
    bool reject_on_runtime_thread{true};
};

#if defined(GALAY_TRACING_ENABLE_OTLP_HTTP)
// Builds a synchronous SpanExporter transport on top of galay-http coroutines.
// Calls block only the caller thread; by default calls from scheduler threads fail fast.
OtlpHttpTransport makeGalayHttpOtlpTransport(GalayHttpOtlpTransportConfig config = {});
#endif

// Exports sampled spans to an OTLP/HTTP JSON endpoint.
// When GALAY_TRACING_ENABLE_OTLP_HTTP is on, the default transport uses galay-http.
class OtlpHttpExporter final : public SpanExporter {
public:
    explicit OtlpHttpExporter(OtlpHttpExporterConfig config = {});
    OtlpHttpExporter(OtlpHttpExporterConfig config, OtlpHttpTransport transport);

    ExportResult exportSpans(std::span<const Span> spans) override;
    bool forceFlush(std::chrono::milliseconds timeout) override;
    bool shutdown(std::chrono::milliseconds timeout) override;

private:
    OtlpHttpExporterConfig m_config;
    std::vector<OtlpHttpHeader> m_headers;
    OtlpHttpTransport m_transport;
};

} // namespace galay::tracing
