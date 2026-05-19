add_library(
    galay-tracing
    common/id_format.cc
    context/context_storage.cc
    context/traceparent.cc
    kernel/span.cc
    kernel/span_guard.cc
    log/console_sink.cc
    log/logger.cc
)
add_library(galay-tracing::galay-tracing ALIAS galay-tracing)

target_compile_features(galay-tracing PUBLIC cxx_std_23)

target_include_directories(
    galay-tracing
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

install(
    TARGETS galay-tracing
    EXPORT galay-tracing-targets
)

install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/galay-tracing/"
    DESTINATION include/galay-tracing
    FILES_MATCHING
        PATTERN "*.h"
)
