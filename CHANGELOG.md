# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/), and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Changed
- BatchSpanProcessor 从 std::deque + mutex 重构为 moodycamel ConcurrentQueue 无锁队列，显著降低 onEnd 延迟
- onEnd 接口签名从 `Span` 改为 `Span&&`，明确所有权转移语义
- 新增 BatchSpanScheduleMode 唤醒策略（kTimed / kOnEnd / kBatchSize），默认 kBatchSize

### Added
- 新增 benchmark B7-BatchProcessorSchedule 和 B8-MoodycamelBatchQueue
- Rust 对比基准新增 R5-RustCrossbeamBatchProcessor 和 R6-RustCrossbeamArrayQueueBatchSize
- 通过 CMake 查找系统安装的 moodycamel ConcurrentQueue 依赖
- 为所有 .cc 实现文件和 .cppm 模块文件补充中文 Doxygen 文件级注释

### Docs
- 补充 schedule_mode 配置说明和性能基准数据

## [v0.2.1] - 2026-05-20

### Docs
- 为所有头文件添加中文 Doxygen 文档注释，包括文件级、类/结构体级、方法级注释以及成员变量行尾注释

## [v0.2.0]

### Note
- Previous release. See git history for details.
