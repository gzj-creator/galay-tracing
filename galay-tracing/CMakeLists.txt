add_library(
    galay-tracing
    common/id_format.cc
    context/context_storage.cc
    context/traceparent.cc
    kernel/batch_span_processor.cc
    kernel/file_span_exporter.cc
    kernel/otlp_http_exporter.cc
    kernel/sampler.cc
    kernel/span.cc
    kernel/span_guard.cc
    log/console_sink.cc
    log/logger.cc
)
add_library(galay-tracing::galay-tracing ALIAS galay-tracing)
set(galay_tracing_install_targets galay-tracing)

find_package(galay-kernel 5.0.0 CONFIG REQUIRED)

set_property(TARGET galay-tracing PROPERTY NO_SYSTEM_FROM_IMPORTED ON)
target_compile_features(galay-tracing PUBLIC cxx_std_23)

target_include_directories(
    galay-tracing
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(galay-tracing PUBLIC galay-kernel::galay-kernel)

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

if(GALAY_TRACING_ENABLE_OTLP_HTTP)
    find_package(galay-http 3.1.0 CONFIG REQUIRED)

    target_compile_definitions(galay-tracing PUBLIC GALAY_TRACING_ENABLE_OTLP_HTTP=1)
    target_link_libraries(galay-tracing PUBLIC galay-http::galay-http)
endif()

if(GALAY_TRACING_ENABLE_KERNEL)
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

set(GALAY_TRACING_CPP23_MODULES_EFFECTIVE FALSE)
if(ENABLE_CPP23_MODULES)
    if(CMAKE_VERSION VERSION_LESS 3.28)
        message(WARNING "ENABLE_CPP23_MODULES requires CMake >= 3.28; disabling module target.")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        message(WARNING "AppleClang is not enabled for galay-tracing modules; disabling module target.")
    else()
        set(GALAY_TRACING_CPP23_MODULES_EFFECTIVE TRUE)
    endif()
endif()

if(GALAY_TRACING_CPP23_MODULES_EFFECTIVE)
    add_library(galay-tracing-modules)
    add_library(galay-tracing::galay-tracing-modules ALIAS galay-tracing-modules)

    target_sources(
        galay-tracing-modules
        PUBLIC
            FILE_SET CXX_MODULES
            FILES module/galay_tracing.cppm
    )
    target_link_libraries(galay-tracing-modules PUBLIC galay-tracing::galay-tracing)

    list(APPEND galay_tracing_install_targets galay-tracing-modules)
endif()

install(TARGETS ${galay_tracing_install_targets} EXPORT galay-tracing-targets)

install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/galay-tracing/"
    DESTINATION include/galay-tracing
    FILES_MATCHING
        PATTERN "*.h"
)
