#include "LogContext.h"

#include <mutex>

namespace {
thread_local std::unordered_map<std::string, std::string> tl_mdc;
}

void XZeroMDC::put(const std::string& key, const std::string& value) {
    tl_mdc[key] = value;
}

void XZeroMDC::remove(const std::string& key) {
    tl_mdc.erase(key);
}

void XZeroMDC::clear() {
    tl_mdc.clear();
}

std::string XZeroMDC::get(const std::string& key) {
    auto it = tl_mdc.find(key);
    return it == tl_mdc.end() ? std::string{} : it->second;
}

std::unordered_map<std::string, std::string> XZeroMDC::all() {
    return tl_mdc;
}
