add_executable(B1-DisabledLog b1_disabled_log.cc)
target_link_libraries(B1-DisabledLog PRIVATE galay-tracing::galay-tracing)

add_executable(B2-EnabledLog b2_enabled_log.cc)
target_link_libraries(B2-EnabledLog PRIVATE galay-tracing::galay-tracing)

add_executable(B3-SpanScope b3_span_scope.cc)
target_link_libraries(B3-SpanScope PRIVATE galay-tracing::galay-tracing)

add_executable(B4-Traceparent b4_traceparent.cc)
target_link_libraries(B4-Traceparent PRIVATE galay-tracing::galay-tracing)
