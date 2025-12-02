#include "LogUtils.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <fstream>
#include <sys/stat.h>

#if defined(_WIN32)
#include <direct.h>
#endif

namespace {

std::string to_lower_copy(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

bool dir_exists(const std::string& path) {
    if (path.empty()) return false;
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}

bool create_single_directory(const std::string& path) {
    if (path.empty() || dir_exists(path)) return true;
#if defined(_WIN32)
    int ret = _mkdir(path.c_str());
#else
    int ret = mkdir(path.c_str(), 0755);
#endif
    if (ret == 0 || (ret == -1 && errno == EEXIST)) {
        return true;
    }
    return false;
}

} // namespace

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
    // POSIX 仅禁止 NUL，其他字符由后续文件系统操作处理
    for (unsigned char ch : input) {
        if (ch == 0) return false;
    }
#endif
    return true;
}

std::string normalized_path(const std::string& input) {
    std::string path = input.empty() ? std::string("log.log") : input;
    const std::size_t last_sep = path.find_last_of("/\\");
    const std::size_t last_dot = path.find_last_of('.');
    bool has_ext = (last_dot != std::string::npos) &&
                   (last_sep == std::string::npos || last_dot > last_sep);
    std::string ext = has_ext ? path.substr(last_dot) : std::string();
    std::string lower_ext = to_lower_copy(ext);

    if (!has_ext) {
        path.append(".log");
    } else if (lower_ext != ".log" && lower_ext != ".txt") {
        path = path.substr(0, last_dot) + ".log";
    }

    // 统一使用 '/' 以兼容跨平台文件流；Windows 也能识别
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

std::size_t safe_file_size(const std::string& path) {
    std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
    if (!file) return 0;
    std::streamoff pos = file.tellg();
    if (pos < 0) return 0;
    return static_cast<std::size_t>(pos);
}

bool file_exists(const std::string& path) {
    std::ifstream f(path.c_str());
    return f.good();
}

bool create_directories(const std::string& dir) {
    if (dir.empty()) return true;

    std::string path = dir;
    std::replace(path.begin(), path.end(), '\\', '/');

    std::string current;
    std::size_t start = 0;

    // 处理绝对路径前缀
    if (path.size() > 1 && path[1] == ':') { // Windows 盘符
        current = path.substr(0, 2);
        start = 2;
        if (path.size() > 2 && path[2] == '/') {
            current += '/';
            start = 3;
        }
    } else if (!path.empty() && path[0] == '/') {
        current = "/";
        start = 1;
    }

    for (std::size_t i = start; i <= path.size(); ++i) {
        if (i == path.size() || path[i] == '/') {
            std::string segment = path.substr(start, i - start);
            if (!segment.empty()) {
                if (!current.empty() && current.back() != '/') {
                    current += '/';
                }
                current += segment;
                if (!create_single_directory(current)) {
                    return false;
                }
            }
            start = i + 1;
        }
    }
    return true;
}

bool ensure_parent_directories(const std::string& filePath) {
    if (filePath.empty()) return true;
    std::size_t pos = filePath.find_last_of("/\\");
    if (pos == std::string::npos || pos == 0) {
        // 相对路径或根目录文件，不需要创建
        return true;
    }
    std::string parent = filePath.substr(0, pos);
    return create_directories(parent);
}
