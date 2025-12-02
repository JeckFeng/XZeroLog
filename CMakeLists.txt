cmake_minimum_required(VERSION 3.10)

# 工程基本信息：名称 / 版本 / 语言
project(XZeroLog VERSION 0.1.0 LANGUAGES CXX)

# 全局编译标准：强制 C++11，禁止编译器扩展，确保可移植性与一致行为
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 生成 compile_commands.json 方便 IDE / clangd 做跳转与补全
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

# 线程库：跨平台支持 std::thread / std::mutex
find_package(Threads REQUIRED)

# 目录变量：源码、头文件、示例
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(DEMO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/demo")

# 核心库源文件（只包含实现文件，头文件通过 target_include_directories 导出）
set(XZEROLOG_SOURCES
    "${SRC_DIR}/FileLogger.cpp"   # 文件/控制台输出、异步、滚动、格式化
    "${SRC_DIR}/LogUtils.cpp"     # 平台探测、路径规范化等工具
    "${SRC_DIR}/LogContext.cpp"   # MDC（traceId/sessionId 等上下文）支持
    "${SRC_DIR}/XZeroLog.cpp"     # 工厂封装入口
)

# 生成静态库 libXZeroLog.a，供外部项目直接链接
add_library(XZeroLog STATIC ${XZEROLOG_SOURCES})
target_link_libraries(XZeroLog PUBLIC Threads::Threads)

# 公开头文件搜索路径：
# - BUILD_INTERFACE：当前构建树使用
# - INSTALL_INTERFACE：安装后使用（若执行 install）
target_include_directories(XZeroLog
    PUBLIC
      $<BUILD_INTERFACE:${INCLUDE_DIR}>
      $<INSTALL_INTERFACE:include>
)

# 可选：按团队规范开启更严格的告警
# target_compile_options(XZeroLog PRIVATE -Wall -Wextra -Wpedantic)

# 开关：是否构建示例可执行（默认 ON）。禁用示例可节省编译时间/依赖。
option(XZEROLOG_BUILD_DEMO "Build demo executable" ON)
if(XZEROLOG_BUILD_DEMO)
    add_executable(xzero_demo
        "${DEMO_DIR}/main.cpp"       # 简单入口
        "${DEMO_DIR}/testLogger.cpp" # 功能覆盖测试
    )
    target_link_libraries(xzero_demo PRIVATE XZeroLog Threads::Threads)
    target_include_directories(xzero_demo PRIVATE "${INCLUDE_DIR}")
endif()

# （可选）安装规则：发布时可启用
# include(GNUInstallDirs)
# install(TARGETS XZeroLog ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
# install(DIRECTORY ${INCLUDE_DIR}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
