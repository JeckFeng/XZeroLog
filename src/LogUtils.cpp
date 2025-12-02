#include "LogUtils.h"

#include <filesystem>

std::string detect_platform() {
#if defined(_WIN32)
    return "Windows";
#elif defined(__linux__)
    return "Linux";
#elif defined(__APPLE__)
    return "macOS";
#else
    return "Unknown";
#endif
}

bool is_path_valid(const std::string& input) {
#if defined(_WIN32)
    // Windows 非法字符列表
    const std::string illegal = "<>:\"|?*";
    for (size_t i = 0; i < input.size(); ++i) {
        const char ch = input[i];
        if (ch == ':' && i == 1) continue; // 允许盘符 "C:"
        if (illegal.find(ch) != std::string::npos) return false;
        if (static_cast<unsigned char>(ch) < 32) return false; // 控制字符
    }
#else
    // POSIX 仅禁止 NUL，其他交由 filesystem 处理
    for (unsigned char ch : input) {
        if (ch == 0) return false;
    }
#endif
    return true;
}

std::string normalized_path(const std::string& input) {
    // 使用 u8path 以更好地支持包含中文的 UTF-8 路径
    std::filesystem::path path = std::filesystem::u8path(input);
    auto ext = path.extension().string();
    if (ext != ".log" && ext != ".txt") {
        path.replace_extension(".log");
    }
    // Windows/Linux 路径均由 filesystem 统一处理
    return path.u8string();
}

std::size_t safe_file_size(const std::filesystem::path& path) {
    std::error_code ec;
    auto size = std::filesystem::file_size(path, ec);
    return ec ? 0 : static_cast<std::size_t>(size);
}
