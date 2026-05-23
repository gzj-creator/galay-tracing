# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/), and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

## [v0.3.0] - 2026-05-23

### Changed
- BatchSpanProcessor 从 std::deque + mutex 重构为 moodycamel ConcurrentQueue 无锁队列，移除 m_mutex / m_condition / m_exportMutex，引入 SpanQueue（无锁队列）+ WorkerControl（原子控制通道），显著降低 onEnd 延迟
- onEnd 接口签名从 `Span` 改为 `Span&&`，明确所有权转移语义
- forceFlush 改为基于 binary_semaphore 的请求-响应模式，支持并发 forceFlush
- shutdown 流程重写：原子 joinClaimed 保证单线程 join，shutdownDrainOk 记录排空结果
- drainQueue 从锁保护改为无锁批量弹出，返回值从 vector 改为 size_t + 复用 buffer

### Added
- 新增 BatchSpanScheduleMode 枚举（kTimed / kOnEnd / kBatchSize），控制后台线程唤醒策略，默认 kBatchSize
- 新增 benchmark B7-BatchProcessorSchedule 和 B8-MoodycamelBatchQueue
- Rust 对比基准新增 R5-RustCrossbeamBatchProcessor 和 R6-RustCrossbeamArrayQueueBatchSize
- 通过 CMake find_path 查找系统安装的 moodycamel ConcurrentQueue 依赖
- 为所有 .cc 实现文件和 .cppm 模块文件补充中文 Doxygen 文件级注释
- 新增 test/t7_batch_processor 单元测试

### Docs
- 补充 schedule_mode 配置说明和性能基准数据
- 更新 traceparent 解析和批量导出到 Collector 的设计文档

## [v0.2.1] - 2026-05-20

### Docs
- 为所有头文件添加中文 Doxygen 文档注释，包括文件级、类/结构体级、方法级注释以及成员变量行尾注释

## [v0.2.0]

### Note
- Previous release. See git history for details.
