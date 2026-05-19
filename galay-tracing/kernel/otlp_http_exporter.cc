#include "galay-tracing/kernel/otlp_http_exporter.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <exception>
#include <memory>
#include <mutex>
#include <string_view>

#if defined(GALAY_TRACING_ENABLE_OTLP_HTTP)
#include "galay-http/kernel/http/http_client.h"
#include "galay-kernel/kernel/runtime.h"

#include <map>
#endif

namespace galay::tracing {

namespace {

[[nodiscard]] bool asciiEqualsIgnoreCase(std::string_view lhs, std::string_view rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        const auto lower_lhs = static_cast<char>(std::tolower(static_cast<unsigned char>(lhs[i])));
        const auto lower_rhs = static_cast<char>(std::tolower(static_cast<unsigned char>(rhs[i])));
        if (lower_lhs != lower_rhs) {
            return false;
        }
    }
    return true;
}

void appendJsonString(std::string& out, std::string_view value) {
    out.push_back('"');
    for (const unsigned char ch : value) {
        switch (ch) {
        case '"':
        case '\\':
            out.push_back('\\');
            out.push_back(static_cast<char>(ch));
            break;
        case '\n':
            out.append("\\n");
            break;
        case '\r':
            out.append("\\r");
            break;
        case '\t':
            out.append("\\t");
            break;
        default:
            if (ch < 0x20) {
                constexpr char kHex[] = "0123456789abcdef";
                out.append("\\u00");
                out.push_back(kHex[ch >> 4]);
                out.push_back(kHex[ch & 0x0f]);
            } else {
                out.push_back(static_cast<char>(ch));
            }
            break;
        }
    }
    out.push_back('"');
}

[[nodiscard]] bool hasHeader(std::span<const OtlpHttpHeader> headers, std::string_view name) {
    return std::ranges::any_of(headers, [name](const OtlpHttpHeader& header) {
        return asciiEqualsIgnoreCase(header.name, name);
    });
}

template <typename Id>
void appendHexId(std::string& out, const Id& id) {
    const auto hex = id.toHexArray();
    out.append(hex.data(), hex.size());
}

[[nodiscard]] std::size_t estimateOtlpJsonBodySize(std::span<const Span> spans) noexcept {
    std::size_t size = 96;
    for (const auto& span : spans) {
        size += 160 + span.name().size();
        const auto& context = span.context();
        if (context.parentSpanId().has_value()) {
            size += SpanId::kHexLength + 18;
        }
        if (!context.tracestate().empty()) {
            size += context.tracestate().size() + 16;
        }
    }
    return size;
}

[[nodiscard]] std::string buildOtlpJsonBody(std::span<const Span> spans) {
    std::string body;
    body.reserve(estimateOtlpJsonBodySize(spans));
    body.append("{\"resourceSpans\":[{\"scopeSpans\":[{\"scope\":{\"name\":\"galay-tracing\"},\"spans\":[");
    for (std::size_t i = 0; i < spans.size(); ++i) {
        const auto& span = spans[i];
        const auto& context = span.context();
        if (i != 0) {
            body.push_back(',');
        }
        body.append("{\"traceId\":\"");
        appendHexId(body, context.traceId());
        body.append("\",\"spanId\":\"");
        appendHexId(body, context.spanId());
        body.append("\",\"name\":");
        appendJsonString(body, span.name());
        body.append(",\"kind\":\"SPAN_KIND_INTERNAL\"");
        if (context.parentSpanId().has_value()) {
            body.append(",\"parentSpanId\":\"");
            appendHexId(body, *context.parentSpanId());
            body.push_back('"');
        }
        if (!context.tracestate().empty()) {
            body.append(",\"traceState\":");
            appendJsonString(body, context.tracestate());
        }
        body.push_back('}');
    }
    body.append("]}]}]}");
    return body;
}

[[nodiscard]] std::vector<OtlpHttpHeader> makeHeaders(const OtlpHttpExporterConfig& config) {
    std::vector<OtlpHttpHeader> headers;
    headers.reserve(config.headers.size() + 1);
    if (!hasHeader(config.headers, "content-type")) {
        headers.push_back({"content-type", "application/json"});
    }
    headers.insert(headers.end(), config.headers.begin(), config.headers.end());
    return headers;
}

[[nodiscard]] OtlpHttpTransport makeUnavailableTransport() {
    return [](OtlpHttpRequest) {
        return OtlpHttpResponse{
            .status_code = 0,
            .error = "GALAY_TRACING_ENABLE_OTLP_HTTP is disabled and no custom OTLP transport was supplied",
        };
    };
}

[[nodiscard]] OtlpHttpTransport makeDefaultTransport() {
#if defined(GALAY_TRACING_ENABLE_OTLP_HTTP)
    return makeGalayHttpOtlpTransport();
#else
    return makeUnavailableTransport();
#endif
}

#if defined(GALAY_TRACING_ENABLE_OTLP_HTTP)
[[nodiscard]] bool hasMappedHeader(const std::map<std::string, std::string>& headers, std::string_view name) {
    return std::ranges::any_of(headers, [name](const auto& entry) {
        return asciiEqualsIgnoreCase(entry.first, name);
    });
}

[[nodiscard]] std::string endpointHostHeader(const galay::http::HttpUrl& url) {
    const bool default_port = (!url.is_secure && url.port == 80) || (url.is_secure && url.port == 443);
    if (default_port) {
        return url.host;
    }
    return url.host + ":" + std::to_string(url.port);
}

galay::kernel::Task<OtlpHttpResponse> sendWithGalayHttp(OtlpHttpRequest request) {
    try {
        const std::string endpoint(request.endpoint);
        auto parsed = galay::http::HttpUrl::parse(endpoint);
        if (!parsed.has_value()) {
            co_return OtlpHttpResponse{.status_code = 0, .error = "invalid OTLP HTTP endpoint"};
        }
        if (parsed->is_secure) {
            co_return OtlpHttpResponse{.status_code = 0, .error = "https OTLP endpoints require a TLS transport"};
        }

        std::string content_type = "application/json";
        std::map<std::string, std::string> headers;
        for (const auto& header : request.headers) {
            if (asciiEqualsIgnoreCase(header.name, "content-type")) {
                content_type = header.value;
                continue;
            }
            headers[header.name] = header.value;
        }
        if (!hasMappedHeader(headers, "host")) {
            headers["Host"] = endpointHostHeader(*parsed);
        }
        if (!hasMappedHeader(headers, "accept")) {
            headers["Accept"] = "application/json";
        }
        if (!hasMappedHeader(headers, "user-agent")) {
            headers["User-Agent"] = "galay-tracing";
        }
        if (!hasMappedHeader(headers, "connection")) {
            headers["Connection"] = "close";
        }

        auto client = galay::http::HttpClientBuilder().build();
        auto connect_result = co_await client.connect(endpoint).timeout(request.timeout);
        if (!connect_result) {
            co_return OtlpHttpResponse{.status_code = 0, .error = connect_result.error().message()};
        }

        auto session = client.getSession();
        auto result = co_await session.post(parsed->path, std::move(request.body), content_type, headers)
            .timeout(request.timeout);
        if (!result) {
            static_cast<void>(co_await client.close());
            co_return OtlpHttpResponse{.status_code = 0, .error = result.error().message()};
        }
        if (!result.value().has_value()) {
            static_cast<void>(co_await client.close());
            co_return OtlpHttpResponse{.status_code = 0, .error = "incomplete OTLP HTTP response"};
        }

        auto response = std::move(result.value().value());
        auto body = response.getBodyStr();
        const int status_code = static_cast<int>(response.header().code());
        static_cast<void>(co_await client.close());
        co_return OtlpHttpResponse{.status_code = status_code, .body = std::move(body)};
    } catch (const std::exception& error) {
        co_return OtlpHttpResponse{.status_code = 0, .error = error.what()};
    } catch (...) {
        co_return OtlpHttpResponse{.status_code = 0, .error = "unknown OTLP HTTP transport error"};
    }
}

class GalayHttpTransportState {
public:
    explicit GalayHttpTransportState(GalayHttpOtlpTransportConfig config)
        : m_config(std::move(config)) {
        m_config.io_scheduler_count = std::max<std::size_t>(m_config.io_scheduler_count, 1);
    }

    OtlpHttpResponse send(OtlpHttpRequest request) {
        if (m_config.reject_on_runtime_thread && galay::kernel::RuntimeHandle::tryCurrent().has_value()) {
            return OtlpHttpResponse{
                .status_code = 0,
                .error = "synchronous OTLP export is not allowed on a galay scheduler thread",
            };
        }

        std::lock_guard lock(m_mutex);
        ensureRuntime();
        auto join = m_runtime->spawn(sendWithGalayHttp(std::move(request)));
        return join.join();
    }

private:
    void ensureRuntime() {
        if (m_runtime) {
            return;
        }

        auto runtime_config = galay::kernel::RuntimeBuilder()
            .ioSchedulerCount(m_config.io_scheduler_count)
            .computeSchedulerCount(0)
            .buildConfig();
        m_runtime = std::make_unique<galay::kernel::Runtime>(runtime_config);
        m_runtime->start();
    }

    GalayHttpOtlpTransportConfig m_config;
    std::mutex m_mutex;
    std::unique_ptr<galay::kernel::Runtime> m_runtime;
};
#endif

} // namespace

#if defined(GALAY_TRACING_ENABLE_OTLP_HTTP)
OtlpHttpTransport makeGalayHttpOtlpTransport(GalayHttpOtlpTransportConfig config) {
    auto state = std::make_shared<GalayHttpTransportState>(std::move(config));
    return [state = std::move(state)](OtlpHttpRequest request) {
        return state->send(std::move(request));
    };
}
#endif

OtlpHttpExporter::OtlpHttpExporter(OtlpHttpExporterConfig config)
    : OtlpHttpExporter(std::move(config), makeDefaultTransport()) {
}

OtlpHttpExporter::OtlpHttpExporter(OtlpHttpExporterConfig config, OtlpHttpTransport transport)
    : m_config(std::move(config)),
      m_headers(makeHeaders(m_config)),
      m_transport(std::move(transport)) {
    if (!m_transport) {
        m_transport = makeUnavailableTransport();
    }
}

ExportResult OtlpHttpExporter::exportSpans(std::span<const Span> spans) {
    if (spans.empty()) {
        return ExportResult::kSuccess;
    }

    OtlpHttpResponse response;
    try {
        response = m_transport(OtlpHttpRequest{
            .method = "POST",
            .endpoint = m_config.endpoint,
            .timeout = m_config.timeout,
            .headers = m_headers,
            .body = buildOtlpJsonBody(spans),
        });
    } catch (...) {
        return ExportResult::kFailure;
    }

    return response.status_code >= 200 && response.status_code < 300
        ? ExportResult::kSuccess
        : ExportResult::kFailure;
}

bool OtlpHttpExporter::forceFlush(std::chrono::milliseconds) {
    return true;
}

bool OtlpHttpExporter::shutdown(std::chrono::milliseconds) {
    return true;
}

} // namespace galay::tracing
