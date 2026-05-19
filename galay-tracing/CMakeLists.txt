add_library(
    galay-tracing
    common/id_format.cc
    context/context_storage.cc
    context/traceparent.cc
    kernel/batch_span_processor.cc
    kernel/file_span_exporter.cc
    kernel/sampler.cc
    kernel/span.cc
    kernel/span_guard.cc
    log/console_sink.cc
    log/logger.cc
)
add_library(galay-tracing::galay-tracing ALIAS galay-tracing)
set(galay_tracing_install_targets galay-tracing)

target_compile_features(galay-tracing PUBLIC cxx_std_23)

target_include_directories(
    galay-tracing
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

if(GALAY_TRACING_ENABLE_SPDLOG)
    find_package(spdlog CONFIG REQUIRED)

    add_library(galay-tracing-spdlog adapters/spdlog_sink.cc)
    add_library(galay-tracing::galay-tracing-spdlog ALIAS galay-tracing-spdlog)

    target_compile_features(galay-tracing-spdlog PUBLIC cxx_std_23)
    target_link_libraries(
        galay-tracing-spdlog
        PUBLIC
            galay-tracing::galay-tracing
            spdlog::spdlog
    )

    list(APPEND galay_tracing_install_targets galay-tracing-spdlog)
endif()

if(GALAY_TRACING_ENABLE_KERNEL)
    find_package(galay-kernel CONFIG REQUIRED)

    add_library(galay-tracing-kernel INTERFACE)
    add_library(galay-tracing::galay-tracing-kernel ALIAS galay-tracing-kernel)

    target_compile_features(galay-tracing-kernel INTERFACE cxx_std_23)
    target_link_libraries(
        galay-tracing-kernel
        INTERFACE
            galay-tracing::galay-tracing
            galay-kernel::galay-kernel
    )

    list(APPEND galay_tracing_install_targets galay-tracing-kernel)
endif()

install(TARGETS ${galay_tracing_install_targets} EXPORT galay-tracing-targets)

install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/galay-tracing/"
    DESTINATION include/galay-tracing
    FILES_MATCHING
        PATTERN "*.h"
)
