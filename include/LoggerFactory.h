#pragma once

#include "LogConfig.h"
#include "Logger.h"
#include <memory>

class LoggerFactory {
public:
    LoggerFactory() = default;
    virtual ~LoggerFactory() = default;
    virtual std::unique_ptr<Logger> InitLogger(const LoggerConfig& config) = 0;
};
