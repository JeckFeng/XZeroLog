#pragma once

#include <string>
#include <unordered_map>

// 线程局部的 MDC（Mapped Diagnostic Context），用于携带 traceId/sessionId 等上下文
namespace XZeroMDC {
// 添加/更新键值
void put(const std::string& key, const std::string& value);
// 移除键
void remove(const std::string& key);
// 清空当前线程上下文
void clear();
// 获取键
std::string get(const std::string& key);
// 获取当前线程全部上下文（拷贝）
std::unordered_map<std::string, std::string> all();
} // namespace XZeroMDC
