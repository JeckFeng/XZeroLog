#pragma once

#include <cstddef>
#include <string>
#include <vector>

// 日志等级定义，便于过滤与格式化
enum class LoggerLevel {
    INFO,
    DEBUG,
    ERROR,
    WARN,
};

// 文件写入模式：追加或覆盖
enum class FileWriteMode {
    Append,
    Overwrite,
};

// 输出格式：人类可读或 JSON
enum class LogFormat {
    HumanFriendly,
    Json,
};

// 用户可配置的日志初始化参数
struct LoggerConfig {
    bool toFile{false};                            // 是否写入文件
    std::string filePath{"log.log"};               // 文件路径，默认使用 .log
    FileWriteMode writeMode{FileWriteMode::Append}; // 文件写入模式：追加/覆盖
    std::string separator{"----------------"};      // 追加模式下的分割线
    // 异步与批量控制
    bool asyncLogging{true};                       // 是否启用异步日志
    std::size_t batchSize{8};                      // 批量写入条数阈值
    std::size_t flushIntervalMs{200};              // 批量写入超时时间（毫秒）
    // 滚动控制
    bool enableRotation{false};                    // 是否开启日志滚动
    std::size_t maxFileSizeBytes{2 * 1024 * 1024}; // 按大小滚动阈值
    std::size_t maxBackupFiles{3};                 // 备份文件数，超出则覆盖最旧
    std::size_t rotationIntervalSeconds{0};        // 按时间滚动间隔，0 表示关闭
    // 格式化选项
    bool includePlatform{true};                    // 是否输出操作系统
    bool includeSource{true};                      // 是否输出源文件/行/函数
    LogFormat logFormat{LogFormat::HumanFriendly}; // 输出格式：HumanFriendly/Json
    bool colorConsole{true};                       // 控制台彩色输出（级别染色）
    bool includeMdc{true};                         // 是否输出 MDC 上下文字段

    bool writeTime{true};                          // 是否写入时间戳
    bool toConsole{true};                          // 是否输出到控制台
    std::vector<LoggerLevel> disableLevels;        // 显式禁止的日志等级
    std::vector<LoggerLevel> onlyLevels;           // 仅允许的日志等级（非空时优先生效）
    bool useErrorCode{true};                       // 是否输出错误码
};
