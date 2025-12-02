#pragma once

#include "LoggerFactory.h"

// 日志工厂：XZeroLog
class XZeroLog : public LoggerFactory {
public:
    XZeroLog() = default;
    ~XZeroLog() override = default;
    std::unique_ptr<Logger> InitLogger(const LoggerConfig& config) override;
};
