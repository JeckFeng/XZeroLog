#pragma once

#include "LogConfig.h"

#include <cstddef>
#include <string>

// 平台探测：用于日志标记
std::string detect_platform();

// 路径合法性校验（支持中文路径）
bool is_path_valid(const std::string& input);

// 规范化日志路径：仅允许 .log/.txt，其余自动改为 .log
std::string normalized_path(const std::string& input);

// 文件大小获取，失败则返回 0
std::size_t safe_file_size(const std::string& path);

// 判断文件是否存在
bool file_exists(const std::string& path);

// 递归创建目录（若目录为空则直接返回 true）
bool create_directories(const std::string& dir);

// 为文件路径创建父目录（若无父目录则返回 true）
bool ensure_parent_directories(const std::string& filePath);
