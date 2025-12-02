#pragma once

// 预定义的常用错误码（可选使用）
enum class XZeroError {
    Ok = 0,
    NotFound = 404,
    NetworkTimeout = 1001,
    DiskFull = 2001,
    PermissionDenied = 2003,
};
