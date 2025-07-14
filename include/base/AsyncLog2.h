#pragma once
#include <cstddef>
#include <string.h>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <fstream>

#include "Thread.h"
#include "LogBuffer.h"

#include <cstdarg>
#include <cstdio>

// 在 AsyncLog2.h 或独立头文件中定义
#define LOG_FORMAT(fmt, ...)                                      \
    do                                                            \
    {                                                             \
        char buffer[4096];                                        \
        snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__);     \
        Asynclogging::instance()->append(buffer, strlen(buffer)); \
    } while (0)

// 定义具体日志级别（如 DEBUG/INFO/ERROR）
#define AsyLOG_DEBUG(fmt, ...) LOG_FORMAT(__FILE__ " [" __LINE__ "] DEBUG: " fmt, ##__VA_ARGS__)
#define AsyLOG_INFO(fmt, ...) LOG_FORMAT(__FILE__ " [" __LINE__ "] INFO: " fmt, ##__VA_ARGS__)
#define AsyLOG_ERROR(fmt, ...) LOG_FORMAT(__FILE__ " [" __LINE__ "] ERROR: " fmt, ##__VA_ARGS__)

class Asynclogging
{
private:


    Asynclogging();
    Asynclogging(const Asynclogging &) = delete;
    Asynclogging &operator=(const Asynclogging &) = delete;

    void threadFunc();

    using BufferVector = std::vector<std::unique_ptr<LogBuffer>>;
    using BufferPtr = BufferVector::value_type;
    void flushToFile(const std::string &buf);
    std::string filename_;
    std::ofstream fout_;

    const int flushInterval_;
    std::atomic<bool> running_;

    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;
    
    static Asynclogging *instance_;
    static std::mutex instanceMutex_;

public:
    static Asynclogging *instance();

    void append(const char *msg, size_t len);
    void open(const std::string &filename);
    void start();
    ~Asynclogging();
};