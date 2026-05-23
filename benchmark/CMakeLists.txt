add_executable(B1-DisabledLog b1_disabled_log.cc)
target_link_libraries(B1-DisabledLog PRIVATE galay-tracing::galay-tracing)

add_executable(B2-EnabledLog b2_enabled_log.cc)
target_link_libraries(B2-EnabledLog PRIVATE galay-tracing::galay-tracing)

add_executable(B3-SpanScope b3_span_scope.cc)
target_link_libraries(B3-SpanScope PRIVATE galay-tracing::galay-tracing)

add_executable(B4-Traceparent b4_traceparent.cc)
target_link_libraries(B4-Traceparent PRIVATE galay-tracing::galay-tracing)

add_executable(B5-OtlpJsonExporter b5_otlp_json_exporter.cc)
target_link_libraries(B5-OtlpJsonExporter PRIVATE galay-tracing::galay-tracing)

add_executable(B6-StructuredEvent b6_structured_event.cc)
target_link_libraries(B6-StructuredEvent PRIVATE galay-tracing::galay-tracing)

add_executable(B7-BatchProcessorSchedule b7_batch_processor_schedule.cc)
target_link_libraries(B7-BatchProcessorSchedule PRIVATE galay-tracing::galay-tracing)

add_executable(B8-MoodycamelBatchQueue b8_moodycamel_batch_queue.cc)
target_include_directories(B8-MoodycamelBatchQueue SYSTEM PRIVATE "${MOODYCAMEL_CONCURRENTQUEUE_INCLUDE_DIR}")
