#include "Logger.h"

#include <iostream>

// 获取日志唯一的实例对象
Logger &Logger::instance()
{
    static Logger logger;
    return logger;
}
// 设置日志等级
void Logger::setLogLevel(int level)
{
    logLevel_ = level;
}
// 写日志地接口     [级别信息] time ： xxxx
void Logger::log(std::string msg)
{
    switch (logLevel_)
    {

    case INFO:
        std::cout << "[INFO]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case FATAL:
        std::cout << "[FATAL]";
        break;
    case DEBUG:
        std::cout << "[DUBUG]";
        break;

    default:
        break;
    }
    // 打印事件和msg
    std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
}