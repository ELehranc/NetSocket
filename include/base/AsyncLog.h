#pragma once

#include "noncopyable.h"
#include "Thread.h"
#include "Timestamp.h"

#include <iostream>
#include <string>
#include <fstream>
#include <mutex>
#include <queue>

#include <condition_variable>
#include <atomic>
#include <memory>
#include <cstdarg>

#define LOG(level, format, ...) \
    AsyncLog::instance()->log(level, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)

class AsyncLog : noncopyable
{

public:
    enum Loglevel
    {

        INFO,  // 正常日志输出
        ERROR, // 出现一些错误
        FATAL, // 毁灭性错误
        DEBUG,  // 调试信息
        LEVEL_COUNT
    };
    void open(const std::string &filename);
    void close();
    void log(Loglevel level, const char *file, int line, const char *func, const char *format, ...);
    void level(Loglevel level);     // 设置日志级别
    static AsyncLog *instance();
    ~AsyncLog();

private:
    AsyncLog();
    AsyncLog(const AsyncLog &) = delete;
    AsyncLog &operator=(const AsyncLog &) = delete;

    void asyncWrite();  // 异步写日志线程

    std::string filename_;
    std::ofstream fout_;

    Loglevel logLevel_ = INFO;
    std::queue<std::string> LogQueue_;

    Thread writethread_;
    Timestamp timestamp_;

    std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic<bool> exit_{false};
    static const char *levels_[LEVEL_COUNT];
    static AsyncLog *instance_;
    static std::mutex instanceMutex_;
};
