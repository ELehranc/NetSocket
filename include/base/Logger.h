#pragma once

#include <string>

#include "noncopyable.h"
#include "Timestamp.h"
// LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFO(logmsgFormat, ...)                  \
    do                                               \
    {                                                \
        Logger &logger = Logger::instance();         \
        logger.setLogLevel(INFO);                    \
        char buf[1024] = {0};                        \
        snprintf(buf, 1024, "[%s:%d] " logmsgFormat, \
                 __FILE__, __LINE__, ##__VA_ARGS__); \
        logger.log(buf);                             \
    } while (0)

// LOG_INFO("%s %d", arg1, arg2)
#define LOG_ERROR(logmsgFormat, ...)                 \
    do                                               \
    {                                                \
        Logger &logger = Logger::instance();         \
        logger.setLogLevel(ERROR);                   \
        char buf[1024] = {0};                        \
        snprintf(buf, 1024, "[%s:%d] " logmsgFormat, \
                 __FILE__, __LINE__, ##__VA_ARGS__); \
        logger.log(buf);                             \
    } while (0)

// LOG_INFO("%s %d", arg1, arg2)
#define LOG_FATAL(logmsgFormat, ...)                 \
    do                                               \
    {                                                \
        Logger &logger = Logger::instance();         \
        logger.setLogLevel(FATAL);                   \
        char buf[1024] = {0};                        \
        snprintf(buf, 1024, "[%s:%d] " logmsgFormat, \
                 __FILE__, __LINE__, ##__VA_ARGS__); \
        logger.log(buf);                             \
        exit(-1);                                    \
    } while (0)
#ifdef MUDEBUG
// LOG_INFO("%s %d", arg1, arg2)
#define LOG_DEBUG(logmsgFormat, ...)                 \
    do                                               \
    {                                                \
        Logger &logger = Logger::instance();         \
        logger.setLogLevel(DEBUG);                   \
        char buf[1024] = {0};                        \
        snprintf(buf, 1024, "[%s:%d] " logmsgFormat, \
                 __FILE__, __LINE__, ##__VA_ARGS__); \
        logger.log(buf);                             \
    } while (0)
#else
#define LOG_DEBUG(logmsgFormat, ...)
#endif
// 定义日志的级别   INFO: 正常日志输出    ERROR：出现一些错误    FATAL：毁灭性错误     DEBUG：调试信息
enum Loglevel
{

    INFO,  // 正常日志输出
    ERROR, // 出现一些错误
    FATAL, // 毁灭性错误
    DEBUG  // 调试信息

};

class Logger : noncopyable
{

public:
    // 获取日志唯一的实例对象
    static Logger &instance();
    // 设置日志等级
    void setLogLevel(int level);
    // 写日志地接口
    void log(std::string msg);

private:
    int logLevel_;
    Logger() {}
};
