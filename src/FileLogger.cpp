#include "FileLogger.h"

#include "LogContext.h"

#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

FileLogger::FileLogger(const LoggerConfig& cfg) : config_(cfg), platform_(detect_platform()) {
    // 将列表转换为集合以便快速过滤
    disabled_.insert(config_.disableLevels.begin(), config_.disableLevels.end());
    only_.insert(config_.onlyLevels.begin(), config_.onlyLevels.end());

    if (config_.toFile) {
        // 规范化路径并校验合法性：仅允许 .log 或 .txt，自动修正后缀，并检测非法字符
        config_.filePath = normalized_path(config_.filePath);
        if (!is_path_valid(config_.filePath)) {
            throw std::runtime_error("日志路径包含非法字符: " + config_.filePath);
        }

        // 若包含父路径则自动创建目录，提升鲁棒性
        if (!ensure_parent_directories(config_.filePath)) {
            throw std::runtime_error("创建日志目录失败: " + config_.filePath);
        }

        const auto mode =
            config_.writeMode == FileWriteMode::Append ? std::ios::app : std::ios::trunc;
        file_.open(config_.filePath.c_str(), std::ios::out | mode);
        if (!file_.is_open()) {
            throw std::runtime_error("无法打开日志文件: " + config_.filePath);
        }

        // 记录当前文件大小与最近滚动时间，便于后续按大小/时间滚动
        current_size_ = safe_file_size(config_.filePath);
        last_rotation_ = std::chrono::system_clock::now();
    }

    // 启动异步写线程：避免高频日志阻塞调用线程
    if (config_.asyncLogging) {
        worker_ = std::thread(&FileLogger::worker_loop, this);
    }
}

FileLogger::~FileLogger() {
    // 通知后台线程退出并 flush
    if (config_.asyncLogging) {
        {
            std::lock_guard<std::mutex> lk(queue_mutex_);
            stop_ = true;
        }
        cv_.notify_one();
        if (worker_.joinable()) {
            worker_.join();
        }
    }
    if (file_.is_open()) {
        file_.close();
    }
}

bool FileLogger::is_enabled(LoggerLevel level) const {
    // only 列表优先；非空时仅允许命中
    if (!only_.empty() && only_.count(level) == 0) {
        return false;
    }
    // 禁用表次之
    if (disabled_.count(level) > 0) {
        return false;
    }
    return true;
}

void FileLogger::ensure_separator_once() const {
    if (!config_.toFile) return;
    if (config_.writeMode != FileWriteMode::Append) return;
    if (separator_written_) return;
    if (!file_.is_open()) return;

    // 追加模式下，在新一轮写入前添加分割线
    file_ << config_.separator << std::endl;
    separator_written_ = true;
}

void FileLogger::log(LoggerLevel level, const std::string& message,
                     int errorCode, const char* file, int line, const char* func) const {
    if (!is_enabled(level)) {
        return;
    }

    const auto level_str = Logger::level_to_string(level);
    const auto tid_hash = std::hash<std::thread::id>{}(std::this_thread::get_id());
    const std::string tid_str = "TID:" + std::to_string(static_cast<std::uint64_t>(tid_hash));

    // 源信息（可选）
    std::string source_str;
    if (config_.includeSource && file) {
        const char* slash = std::strrchr(file, '/');
        const char* backslash = std::strrchr(file, '\\');
        const char* last_sep = slash ? (backslash && backslash > slash ? backslash : slash) : backslash;
        const char* base = last_sep ? last_sep + 1 : file;
        std::ostringstream ss;
        ss << base;
        if (line > 0) ss << ":" << line;
        if (func && *func) ss << " " << func;
        source_str = ss.str();
    }

    // MDC 上下文
    std::unordered_map<std::string, std::string> mdc;
    if (config_.includeMdc) {
        mdc = XZeroMDC::all();
    }

    auto escape_json = [](const std::string& in) {
        std::ostringstream e;
        for (char c : in) {
            switch (c) {
            case '\"': e << "\\\""; break;
            case '\\': e << "\\\\"; break;
            case '\n': e << "\\n"; break;
            case '\r': e << "\\r"; break;
            case '\t': e << "\\t"; break;
            default: e << c; break;
            }
        }
        return e.str();
    };

    std::string formatted;
    if (config_.logFormat == LogFormat::Json) {
        std::ostringstream oss;
        oss << "{";
        oss << "\"timestamp\":\"" << (config_.writeTime ? Logger::current_time_iso8601_utc() : "") << "\",";
        oss << "\"OS\":\"" << (config_.includePlatform ? escape_json(platform_) : "") << "\",";
        oss << "\"level\":\"" << level_str << "\",";
        oss << "\"thread\":\"" << tid_str << "\"";
        if (!source_str.empty()) {
            oss << ",\"logger\":\"" << escape_json(source_str) << "\"";
        }
        oss << ",\"message\":\"" << escape_json(message) << "\"";
        if (config_.includeMdc && !mdc.empty()) {
            oss << ",\"context\":{";
            bool first = true;
            for (const auto& kv : mdc) {
                if (!first) oss << ",";
                oss << "\"" << escape_json(kv.first) << "\":\"" << escape_json(kv.second) << "\"";
                first = false;
            }
            oss << "}";
        }
        if (config_.useErrorCode) {
            oss << ",\"error_code\":" << errorCode;
        }
        oss << "}";
        formatted = oss.str();
    } else {
        std::ostringstream oss;
        if (config_.writeTime) {
            oss << "[" << Logger::current_time() << "] ";
        }
        if (config_.includePlatform) {
            oss << "[" << platform_ << "] ";
        }
        oss << "[" << std::left << std::setw(6) << level_str << std::right << "] ";
        oss << "[" << tid_str << "] ";
        if (!source_str.empty()) {
            oss << "(" << source_str << ") - ";
        }
        oss << message;
        if (!mdc.empty()) {
            oss << " [CTX:";
            bool first = true;
            for (const auto& kv : mdc) {
                if (!first) oss << " ";
                oss << kv.first << "=" << kv.second;
                first = false;
            }
            oss << "]";
        }
        if (config_.useErrorCode) {
            oss << " (Error Code: " << errorCode << ")";
        }
        formatted = oss.str();
    }

    if (config_.asyncLogging) {
        // 将日志放入队列，后台线程批量写入
        {
            std::lock_guard<std::mutex> lk(queue_mutex_);
            queue_.push_back(LogItem{formatted, level});
        }
        cv_.notify_one();
    } else {
        // 同步路径，直接输出
        std::lock_guard<std::mutex> lock(io_mutex_);
        rotate_if_needed(formatted.size() + 1);
        ensure_separator_once();
        write_line_unlocked(formatted, level);
    }
}

void FileLogger::write_line_unlocked(const std::string& line, LoggerLevel level) const {
    if (config_.toConsole) {
        if (config_.colorConsole) {
            const char* color = nullptr;
            switch (level) {
            case LoggerLevel::ERROR: color = "\033[31m"; break;
            case LoggerLevel::WARN:  color = "\033[33m"; break;
            case LoggerLevel::INFO:  color = "\033[32m"; break;
            case LoggerLevel::DEBUG: color = "\033[36m"; break;
            default: color = ""; break;
            }
            std::cout << color << line << "\033[0m" << std::endl;
        } else {
            std::cout << line << std::endl;
        }
    }
    if (config_.toFile && file_.is_open()) {
        file_ << line << std::endl;
        current_size_ += line.size() + 1; // 维护当前文件大小
        if (!file_) {
            throw std::runtime_error("写入日志文件失败: " + config_.filePath);
        }
    }
}

void FileLogger::worker_loop() {
    std::vector<FileLogger::LogItem> batch;
    batch.reserve(config_.batchSize);
    const auto wait_duration = std::chrono::milliseconds(config_.flushIntervalMs);

    std::unique_lock<std::mutex> lk(queue_mutex_);
    while (true) {
        cv_.wait_for(lk, wait_duration, [&] {
            return stop_ || queue_.size() >= config_.batchSize || !queue_.empty();
        });

        if (queue_.empty() && stop_) {
            break;
        }

        while (!queue_.empty() && batch.size() < config_.batchSize) {
            batch.push_back(std::move(queue_.front()));
            queue_.pop_front();
        }

        lk.unlock();
        if (!batch.empty()) {
            std::lock_guard<std::mutex> io_lock(io_mutex_);
            for (const auto& line : batch) {
                rotate_if_needed(line.text.size() + 1);
                ensure_separator_once();
                write_line_unlocked(line.text, line.level);
            }
            batch.clear();
        }
        lk.lock();
    }

    // 退出前 flush 剩余
    while (!queue_.empty()) {
        batch.push_back(std::move(queue_.front()));
        queue_.pop_front();
    }
    lk.unlock();

    if (!batch.empty()) {
        std::lock_guard<std::mutex> io_lock(io_mutex_);
        for (const auto& line : batch) {
            rotate_if_needed(line.text.size() + 1);
            ensure_separator_once();
            write_line_unlocked(line.text, line.level);
        }
    }
}

void FileLogger::rotate_if_needed(std::size_t next_line_len) const {
    if (!config_.enableRotation || !config_.toFile || !file_.is_open()) return;

    bool need_rotate = false;
    const auto now = std::chrono::system_clock::now();

    if (config_.maxFileSizeBytes > 0 &&
        current_size_ + next_line_len > config_.maxFileSizeBytes) {
        need_rotate = true;
    }
    if (!need_rotate && config_.rotationIntervalSeconds > 0) {
        const auto interval = std::chrono::seconds(config_.rotationIntervalSeconds);
        if (now - last_rotation_ >= interval) {
            need_rotate = true;
        }
    }
    if (need_rotate) {
        rotate_files();
        last_rotation_ = now;
    }
}

void FileLogger::rotate_files() const {
    if (!config_.toFile) return;

    if (file_.is_open()) {
        file_.close();
    }

    // 备份：log -> log.1, log.1 -> log.2 ...
    if (config_.maxBackupFiles > 0) {
        for (std::size_t i = config_.maxBackupFiles; i > 0; --i) {
            const std::string target = config_.filePath + "." + std::to_string(i);
            const std::string source =
                (i == 1) ? config_.filePath : config_.filePath + "." + std::to_string(i - 1);

            std::remove(target.c_str()); // 删除已有备份
            if (file_exists(source)) {
                std::rename(source.c_str(), target.c_str());
            }
        }
    } else {
        std::remove(config_.filePath.c_str());
    }

    // 重新打开主文件，重置大小与分割线状态
    file_.open(config_.filePath.c_str(), std::ios::out | std::ios::trunc);
    current_size_ = 0;
    separator_written_ = false;
    if (!file_.is_open()) {
        throw std::runtime_error("滚动后无法重新打开日志文件: " + config_.filePath);
    }
}
