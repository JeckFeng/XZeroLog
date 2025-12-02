#pragma once

#include "LogConfig.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

// 基础日志接口，提供等级转换与时间获取工具
class Logger {
public:
    Logger() = default;
    virtual ~Logger() = default;

    // 线程安全由具体实现保证；接口保持 const，便于并发调用
    virtual void log(LoggerLevel level, const std::string& message,
                     int errorCode = 0,
                     const char* file = nullptr,
                     int line = 0,
                     const char* func = nullptr) const = 0;

    static std::string level_to_string(LoggerLevel level) {
        switch (level) {
        case LoggerLevel::INFO:
            return "INFO";
        case LoggerLevel::ERROR:
            return "ERROR";
        case LoggerLevel::WARN:
            return "WARN";
        case LoggerLevel::DEBUG:
            return "DEBUG";
        default:
            return "UNKNOWN";
        }
    }

    // 使用 chrono + put_time 获取线程安全的时间戳
    static std::string current_time() {
        const auto now = std::chrono::system_clock::now();
        const auto time = std::chrono::system_clock::to_time_t(now);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now.time_since_epoch()) %
                        1000;
        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
            << "." << std::setw(3) << std::setfill('0') << ms.count();
        return oss.str();
    }

    // UTC ISO8601 带毫秒，适用于 JSON
    static std::string current_time_iso8601_utc() {
        const auto now = std::chrono::system_clock::now();
        const auto time = std::chrono::system_clock::to_time_t(now);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now.time_since_epoch()) %
                        1000;
        std::tm tm{};
#if defined(_WIN32)
        gmtime_s(&tm, &time);
#else
        gmtime_r(&time, &tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S")
            << "." << std::setw(3) << std::setfill('0') << ms.count()
            << "Z";
        return oss.str();
    }
};

// 便捷宏：自动捕获文件/行/函数，避免用户手填
#define XZERO_LOG(logger, level, message, errorCode) \
    (logger)->log((level), (message), (errorCode), __FILE__, __LINE__, __func__)

// 级别便捷别名，默认 errorCode = 0
#define XZERO_INFO(logger, message) \
    XZERO_LOG((logger), LoggerLevel::INFO, (message), 0)
#define XZERO_WARN(logger, message) \
    XZERO_LOG((logger), LoggerLevel::WARN, (message), 0)
#define XZERO_DEBUG(logger, message) \
    XZERO_LOG((logger), LoggerLevel::DEBUG, (message), 0)
#define XZERO_ERROR(logger, message) \
    XZERO_LOG((logger), LoggerLevel::ERROR, (message), 0)

// 带错误码的快捷宏
#define XZERO_INFO_E(logger, message, err) \
    XZERO_LOG((logger), LoggerLevel::INFO, (message), (err))
#define XZERO_WARN_E(logger, message, err) \
    XZERO_LOG((logger), LoggerLevel::WARN, (message), (err))
#define XZERO_DEBUG_E(logger, message, err) \
    XZERO_LOG((logger), LoggerLevel::DEBUG, (message), (err))
#define XZERO_ERROR_E(logger, message, err) \
    XZERO_LOG((logger), LoggerLevel::ERROR, (message), (err))
