#include "AsyncLog.h"
#include <cstring>
#include <stdexcept>

const char *AsyncLog::levels_[LEVEL_COUNT] = {
    "DEBUG",
    "INFO",
    "ERROR",
    "FATAL"};

AsyncLog *AsyncLog::instance_ = nullptr;
std::mutex AsyncLog::instanceMutex_;

AsyncLog *AsyncLog::instance()
{

    if (instance_ == nullptr)
    {

        std::lock_guard<std::mutex> gd(instanceMutex_);
        if (instance_ == nullptr)
        {
            instance_ = new AsyncLog();
        }
    }
    return instance_;
}

AsyncLog::AsyncLog() : writethread_(std::bind(&AsyncLog::asyncWrite, this))
{
    writethread_.start();
}
AsyncLog::~AsyncLog()
{

    {
        std::unique_lock<std::mutex> lock(mutex_);
        exit_ = true;
    }
    condition_.notify_all();

    writethread_.join();

    close();
}

void AsyncLog::open(const std::string &filename)
{

    std::lock_guard<std::mutex> lock(mutex_);
    filename_ = filename;
    fout_.open(filename, std::ios::trunc);
    if (fout_.fail())
    {
        throw std::logic_error("Failed to open file: " + filename);
    }
}

void AsyncLog::close()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (fout_.is_open())
    {
        fout_.close();
    }
}
void AsyncLog::log(Loglevel level, const char *file, int line, const char *func, const char *format, ...)
{
    if (level < logLevel_)
    {
        return;
    }

    timestamp_.now().toString();

    const char *filename = strrchr(file, '/');
    if (!filename)
    {
        filename = strrchr(file, '\\');
    }
    filename = filename ? filename + 1 : file;

    char content[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(content, sizeof(content), format, args);
    va_end(args);

    // 拼接日志完整信息
    char logMessage[2048];
    snprintf(logMessage, sizeof(logMessage), "[%s %s(%s),%d]:%s", timestamp_.now().toString().c_str(), filename, func, line, content);

    {

        std::lock_guard<std::mutex> lock(mutex_);
        LogQueue_.push(logMessage);
    }
    condition_.notify_one();
}
void AsyncLog::level(Loglevel level)
{
    logLevel_ = level;
} // 设置日志级别

void AsyncLog::asyncWrite()
{
    while (true)
    {

        std::string logMessage;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this]()
                            { return exit_ || !LogQueue_.empty(); });
            if (exit_ && LogQueue_.empty())
            {

                break;
            }
            logMessage = LogQueue_.front();
            LogQueue_.pop();
        }
        if (fout_.is_open())
        {
            fout_ << logMessage;
            fout_.flush();
        }
        else
        {
            std::cerr << logMessage;
        }
    }

} // 异步写日志线程
