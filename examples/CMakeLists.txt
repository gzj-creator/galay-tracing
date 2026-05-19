add_executable(E1-LogContext include/e1_log_context.cc)
target_link_libraries(E1-LogContext PRIVATE galay-tracing::galay-tracing)

add_executable(E2-Traceparent include/e2_traceparent.cc)
target_link_libraries(E2-Traceparent PRIVATE galay-tracing::galay-tracing)

add_executable(E3-OtlpExporter include/e3_otlp_exporter.cc)
target_link_libraries(E3-OtlpExporter PRIVATE galay-tracing::galay-tracing)

if(TARGET galay-tracing::galay-tracing-modules)
    add_executable(E1-LogContextImport import/e1_log_context.cc)
    target_link_libraries(E1-LogContextImport PRIVATE galay-tracing::galay-tracing-modules)

    add_executable(E2-TraceparentImport import/e2_traceparent.cc)
    target_link_libraries(E2-TraceparentImport PRIVATE galay-tracing::galay-tracing-modules)

    add_executable(E3-OtlpExporterImport import/e3_otlp_exporter.cc)
    target_link_libraries(E3-OtlpExporterImport PRIVATE galay-tracing::galay-tracing-modules)
endif()
