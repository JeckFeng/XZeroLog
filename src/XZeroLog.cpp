#include "XZeroLog.h"

#include "FileLogger.h"

std::unique_ptr<Logger> XZeroLog::InitLogger(const LoggerConfig& config) {
    // 直接传入用户配置，构造线程安全的 FileLogger
    return std::unique_ptr<Logger>(new FileLogger(config));
}
