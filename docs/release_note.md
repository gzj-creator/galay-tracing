# Release Note

按时间顺序追加版本记录，避免覆盖历史发布说明。

## v0.2.0 - 2026-05-20

- 版本级别：中版本（minor）
- Git 提交消息：`feat: 增加 tracing 库级 BaseLogger 日志入口`
- Git Tag：`v0.2.0`
- 自述摘要：
  - 收束前序 tracing 功能线：trace/span 标识、W3C traceparent、上下文存储、Span/SpanGuard、业务关联日志、结构化事件、sampler、batch processor、file exporter、OTLP/HTTP JSON exporter、可选 spdlog sink、可选 kernel context adapter、示例、测试与 benchmark。
  - 新增 `galay::tracing::log::set/get`，用于 tracing 库内部诊断日志，底层使用 `galay-kernel` 的 `BaseLogger` 和独立 logger 槽位。
  - 新增 `TRACING_LOG_*` 内部诊断宏，并在 batch span processor 与 OTLP/HTTP exporter 的失败和调试路径埋点。
  - 保留 tracing 业务层 `Logger` / `LogSink` / `GALAY_LOG_*` 日志关联模型，避免内部诊断 logger 影响用户业务日志 sink。
  - 新增 `T10-base_logger` 测试，验证未设置 logger 和级别过滤时不会求值日志格式化参数。
  - 将 CMake package 版本提升到 `0.2.0`，依赖对齐 `galay-kernel 5.0.0`，可选 OTLP/HTTP 路径对齐 `galay-http 3.1.0`。
