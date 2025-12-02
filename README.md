# XZeroLog 日志系统 🤖

## C++ 标准支持
- 需 CMake ≥ 3.10，C++11 编译器（已移除 `std::filesystem` 依赖，使用跨平台轻量实现）。

## 功能概览
- 同时输出文件/控制台，线程安全，异步+批量写入，减少阻塞。
- 日志滚动（按大小/时间）+ 备份保留。
- 两种格式：
  - Human-Friendly（紧凑易读，含毫秒时间、OS、线程、源信息、错误码）。
  - JSON（结构化，便于机器解析，含上下文字段）。
- 上下文 MDC（traceId/sessionId 等）自动注入。
- 控制台彩色输出（可关），可选源信息/平台/时间。
- 路径规范化与自动建目录，支持中文路径（Windows 侧依赖 UTF-8 配置）。
- 自定义错误码输出。

## 主要配置（LoggerConfig）
| 字段 | 含义 | 默认 |
| --- | --- | --- |
| `toFile` | 是否写文件 | false |
| `filePath` | 日志文件路径（自动补 .log/.txt） | `log.log` |
| `writeMode` | 追加/覆盖 | Append |
| `separator` | 追加模式分割线 | `----------------` |
| `asyncLogging` | 异步 | true |
| `batchSize` / `flushIntervalMs` | 批量阈值/超时 | 8 / 200 |
| `enableRotation` | 开启滚动 | false |
| `maxFileSizeBytes` | 按大小滚动阈值 | 2MB |
| `maxBackupFiles` | 备份数 | 3 |
| `rotationIntervalSeconds` | 按时间滚动间隔（0 关闭） | 0 |
| `includePlatform` / `includeSource` / `includeMdc` | 是否输出 OS / 源信息 / MDC | true |
| `logFormat` | `HumanFriendly` 或 `Json` | HumanFriendly |
| `colorConsole` | 控制台彩色 | true |
| `writeTime` / `toConsole` / `useErrorCode` | 时间/控制台/错误码输出 | true |
| `disableLevels` / `onlyLevels` | 等级过滤 | 空 |

## C++11 兼容说明
- 移除 C++17 `std::filesystem` 依赖，目录创建、文件检查与路径规范化采用跨平台轻量实现。
- 使用 C++11 可用的智能指针/线程接口，避免 `std::make_unique` 等较新特性。
- Linux/Windows 均可编译，线程库通过 CMake `Threads` 包自动链接。

## 便捷宏与用法
- 基础：`XZERO_LOG(logger, level, msg, errCode)` 自动带文件/行/函数。
- 无错误码别名：`XZERO_INFO/WARN/DEBUG/ERROR(logger, msg)`
- 带错误码别名：`XZERO_INFO_E/WARN_E/DEBUG_E/ERROR_E(logger, msg, err)`

示例：
```cpp
auto logger = XZeroLog().InitLogger(cfg);
XZERO_INFO(logger, "服务启动");
XZERO_ERROR_E(logger, "磁盘不足", static_cast<int>(XZeroError::DiskFull));
```

## 上下文 MDC（trace/session 等）
```cpp
#include "LogContext.h"
XZeroMDC::put("traceId", "trace-123");
XZeroMDC::put("sessionId", "sess-xyz");
XZERO_INFO(logger, "携带上下文的日志");
XZeroMDC::clear();
```
Human-Friendly 会输出 `[CTX:traceId=... sessionId=...]`，JSON 会输出 `"context":{"traceId":"..."...}`。

## 自定义错误码
- 可直接传入 `int`，或使用预置枚举 `XZeroError`（可选，见 `include/XZeroError.h`）。
```cpp
XZERO_ERROR_E(logger, "网络超时", static_cast<int>(XZeroError::NetworkTimeout));
```

## JSON vs Human-Friendly
- Human-Friendly 示例：`[2025-12-01 16:50:48.596] [Linux] [ERROR ] [TID:...] (file.cpp:120 func) - msg (Error Code: 1001)`
- JSON 示例：`{"timestamp":"...Z","OS":"Linux","level":"INFO","thread":"TID:...","logger":"file.cpp:120 func","message":"msg","context":{...},"error_code":1001}`
切换方式：`cfg.logFormat = LogFormat::Json;`

## 编译与使用 🚀
默认生成静态库 `libXZeroLog.a`，并编译示例可执行 `xzero_demo`。

**构建静态库（含示例）：**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build        # 生成 libXZeroLog.a 和 xzero_demo
```

**只构建静态库（不编译示例）：**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DXZEROLOG_BUILD_DEMO=OFF
cmake --build build
```

**运行示例：**
```bash
./build/xzero_demo
```

**在其他项目中使用（add_subdirectory）：**
```cmake
add_subdirectory(/path/to/XZeroLog)
target_link_libraries(your_target PRIVATE XZeroLog)
target_include_directories(your_target PRIVATE /path/to/XZeroLog/include)
```

**通过安装前缀使用（可选）：**
```bash
cmake --install build --prefix /opt/xzerolog
# 然后在项目中 find_package 或直接 include/link 安装路径下的头文件与库
```

尽情享用 XZeroLog！ 🎉
