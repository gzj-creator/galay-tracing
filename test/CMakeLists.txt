add_executable(T1-package_surface t1_package_surface.cc)
target_link_libraries(T1-package_surface PRIVATE galay-tracing::galay-tracing)

add_test(NAME T1-package_surface COMMAND T1-package_surface)

add_executable(T2-trace_ids t2_trace_ids.cc)
target_link_libraries(T2-trace_ids PRIVATE galay-tracing::galay-tracing)

add_test(NAME T2-trace_ids COMMAND T2-trace_ids)

add_executable(T3-traceparent t3_traceparent.cc)
target_link_libraries(T3-traceparent PRIVATE galay-tracing::galay-tracing)

add_test(NAME T3-traceparent COMMAND T3-traceparent)

add_executable(T4-span_guard t4_span_guard.cc)
target_link_libraries(T4-span_guard PRIVATE galay-tracing::galay-tracing)

add_test(NAME T4-span_guard COMMAND T4-span_guard)

add_executable(T5-logger_context t5_logger_context.cc)
target_link_libraries(T5-logger_context PRIVATE galay-tracing::galay-tracing)

add_test(NAME T5-logger_context COMMAND T5-logger_context)
