#pragma once

#include "LogConfig.h"
#include "LogUtils.h"
#include "Logger.h"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <mutex>
#include <thread>
#include <unordered_set>

// 线程安全的可配置日志器：支持文件/控制台输出、等级过滤、时间戳、错误码、滚动与异步批量
class FileLogger : public Logger {
public:
    explicit FileLogger(const LoggerConfig& cfg);
    ~FileLogger();

    void log(LoggerLevel level, const std::string& message,
             int errorCode = 0,
             const char* file = nullptr,
             int line = 0,
             const char* func = nullptr) const override;

private:
    bool is_enabled(LoggerLevel level) const;
    void ensure_separator_once() const;
    void write_line_unlocked(const std::string& line, LoggerLevel level) const;
    void worker_loop();
    void rotate_if_needed(std::size_t next_line_len) const;
    void rotate_files() const;

    LoggerConfig config_;
    mutable std::ofstream file_;
    mutable std::mutex io_mutex_;      // 保护文件/控制台输出
    mutable std::mutex queue_mutex_;   // 保护队列
    mutable std::condition_variable cv_;
    struct LogItem {
        std::string text;
        LoggerLevel level;
    };
    mutable std::deque<LogItem> queue_;
    mutable bool stop_{false};
    mutable bool separator_written_{false};
    std::thread worker_;

    std::unordered_set<LoggerLevel> disabled_;
    std::unordered_set<LoggerLevel> only_;
    std::string platform_;
    mutable std::size_t current_size_{0};
    mutable std::chrono::system_clock::time_point last_rotation_;
};
