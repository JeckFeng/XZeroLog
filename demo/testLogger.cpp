#include "Logger.h"
#include "XZeroLog.h"
#include "LogContext.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// 简单的测试入口，覆盖主要特性
void run_all_tests() {
    std::cout << "=== Logger Tests Start ===" << std::endl;

    // 1) 基本配置测试：自定义路径、时间戳、OS、线程ID、错误码
    {
        LoggerConfig cfg;
        cfg.toFile = true;
        cfg.filePath = "build/logs/basic.log";
        cfg.writeTime = true;
        cfg.useErrorCode = true;
        cfg.toConsole = true; // 避免干扰输出
        XZeroLog factory;
        auto logger = factory.InitLogger(cfg);
        XZERO_LOG(logger, LoggerLevel::INFO, "配置测试：含时间戳、OS、线程ID", 100);
        XZERO_LOG(logger, LoggerLevel::WARN, "配置测试：含时间戳、OS、线程ID", 100);
        XZERO_LOG(logger, LoggerLevel::DEBUG, "配置测试：含时间戳、OS、线程ID", 100);
        XZERO_LOG(logger, LoggerLevel::ERROR, "配置测试：含时间戳、OS、线程ID", 100);
        XZERO_DEBUG_E(logger, "配置测试：含时间戳、OS、线程ID", 300);
    }

    // 2) 异步日志 + 批量：设置小批量和短 flush 间隔
    {
        LoggerConfig cfg;
        cfg.toFile = true;
        cfg.filePath = "build/logs/async_batch.log";
        cfg.asyncLogging = true;
        cfg.batchSize = 4;
        cfg.flushIntervalMs = 100;
        cfg.toConsole = false;
        XZeroLog factory;
        auto logger = factory.InitLogger(cfg);
        for (int i = 0; i < 10; ++i) {
            XZERO_LOG(logger, LoggerLevel::DEBUG, "异步批量日志，第" + std::to_string(i) + "条", 0);
        }
        // 等待后台线程 flush
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    // 3) 多线程并发写入测试：多个线程同时写日志
    {
        LoggerConfig cfg;
        cfg.toFile = true;
        cfg.filePath = "build/logs/multithread.log";
        cfg.asyncLogging = true;
        cfg.toConsole = false;
        XZeroLog factory;
        auto logger = factory.InitLogger(cfg);

        auto worker = [&logger](int id) {
            for (int i = 0; i < 5; ++i) {
                XZERO_LOG(logger, LoggerLevel::INFO,
                          "线程并发测试：线程" + std::to_string(id) +
                              " 第" + std::to_string(i) + "条",
                          0);
            }
        };
        std::vector<std::thread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back(worker, t);
        }
        for (auto& th : threads) {
            th.join();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // 4) 日志滚动与备份：设置较小大小和备份个数，写多条触发滚动
    {
        LoggerConfig cfg;
        cfg.toFile = true;
        cfg.filePath = "build/logs/rolling.log";
        cfg.enableRotation = true;
        cfg.maxFileSizeBytes = 1024;  // 1KB 触发大小滚动
        cfg.maxBackupFiles = 2;       // 保留 2 个备份
        cfg.toConsole = false;
        cfg.asyncLogging = true;
        XZeroLog factory;
        auto logger = factory.InitLogger(cfg);
        for (int i = 0; i < 200; ++i) {
            XZERO_LOG(logger, LoggerLevel::INFO,
                      "滚动测试：第" + std::to_string(i) +
                          "条，触发文件大小滚动与备份",
                      0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // 5) 批量写入 + 分割线验证：使用追加模式与自定义分割线
    {
        LoggerConfig cfg;
        cfg.toFile = true;
        cfg.filePath = "build/logs/batch_separator.log";
        cfg.writeMode = FileWriteMode::Append;
        cfg.separator = "==== NEW SESSION ====";
        cfg.batchSize = 3;
        cfg.asyncLogging = true;
        cfg.toConsole = false;
        XZeroLog factory;
        auto logger = factory.InitLogger(cfg);
        for (int i = 0; i < 9; ++i) {
            XZERO_LOG(logger, LoggerLevel::WARN, "批量+分割线测试 第" + std::to_string(i) + "条", 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    // 6) 格式化选项：线程ID、时间戳、编码标记
    {
        LoggerConfig cfg;
        cfg.toFile = true;
        cfg.filePath = "build/logs/format.log";
        cfg.writeTime = true;
        cfg.toConsole = false;
        XZeroLog factory;
        auto logger = factory.InitLogger(cfg);
        XZERO_LOG(logger, LoggerLevel::INFO, "格式化测试：包含线程ID与时间戳", 0);
        XZERO_LOG(logger, LoggerLevel::INFO, u8"格式化测试：中文UTF-8编码", 0);
    }

    // 7) JSON 格式输出
    {
        LoggerConfig cfg;
        cfg.toFile = true;
        cfg.filePath = "build/logs/json.log";
        cfg.logFormat = LogFormat::Json;
        cfg.useErrorCode = true;
        cfg.toConsole = false;
        XZeroLog factory;
        auto logger = factory.InitLogger(cfg);
        XZERO_LOG(logger, LoggerLevel::INFO, "Json 格式测试：用户未找到", 404);
        XZERO_LOG(logger, LoggerLevel::ERROR, "Json 格式测试：磁盘空间不足", 2001);
    }

    // 8) MDC 上下文：traceId / sessionId 自动注入
    {
        LoggerConfig cfg;
        cfg.toFile = true;
        cfg.filePath = "build/logs/mdc.log";
        cfg.logFormat = LogFormat::Json;
        cfg.includeMdc = true;
        cfg.toConsole = false;
        XZeroLog factory;
        auto logger = factory.InitLogger(cfg);

        XZeroMDC::put("traceId", "trace-abc-001");
        XZeroMDC::put("sessionId", "sess-xyz");
        XZERO_LOG(logger, LoggerLevel::INFO, "MDC 测试：携带 trace/session", 0);
        XZeroMDC::clear();
    }

    // 9) 自定义错误码：用户只需定义枚举/常量并在 log 第三个参数传入即可
    {
        enum class AppError {
            Ok = 0,
            NetworkTimeout = 1001,
            DiskFull = 2001,
        };
        LoggerConfig cfg;
        cfg.toFile = true;
        cfg.filePath = "build/logs/custom_error.log";
        cfg.useErrorCode = true;     // 开启错误码输出
        cfg.toConsole = false;
        XZeroLog factory;
        auto logger = factory.InitLogger(cfg);

        // 用户只需传入自定义错误码（转换为 int 即可），接口保持简单
        XZERO_LOG(logger, LoggerLevel::ERROR, "自定义错误码测试：网络超时",
                  static_cast<int>(AppError::NetworkTimeout));
        XZERO_LOG(logger, LoggerLevel::ERROR, "自定义错误码测试：磁盘空间不足",
                  static_cast<int>(AppError::DiskFull));
    }

    std::cout << "=== Logger Tests Done ===" << std::endl;
}
