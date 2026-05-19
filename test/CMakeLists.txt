add_executable(T1-package_surface t1_package_surface.cc)
target_link_libraries(T1-package_surface PRIVATE galay-tracing::galay-tracing)

add_test(NAME T1-package_surface COMMAND T1-package_surface)

add_executable(T2-trace_ids t2_trace_ids.cc)
target_link_libraries(T2-trace_ids PRIVATE galay-tracing::galay-tracing)

add_test(NAME T2-trace_ids COMMAND T2-trace_ids)
