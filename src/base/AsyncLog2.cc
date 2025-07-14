#include "AsyncLog2.h"
#include "Logger.h"

Asynclogging::Asynclogging() : thread_(std::bind(&Asynclogging::threadFunc, this), "log"),
                               running_(false),
                               flushInterval_(3),
                               mutex_(),
                               cond_(),
                               currentBuffer_(new LogBuffer),
                               nextBuffer_(new LogBuffer),
                               buffers_()
{
    buffers_.reserve(16);
}

Asynclogging *Asynclogging::instance_ = nullptr;
std::mutex Asynclogging::instanceMutex_;

Asynclogging *Asynclogging::instance()
{

    if (instance_ == nullptr)
    {

        std::lock_guard<std::mutex> gd(instanceMutex_);
        if (instance_ == nullptr)
        {
            instance_ = new Asynclogging();
        }
    }
    return instance_;
}

Asynclogging::~Asynclogging()
{
    running_ = false;
    cond_.notify_all();
    thread_.join();
    if (currentBuffer_->length() > 0)
    {
        flushToFile(currentBuffer_->data());
    }
}

void Asynclogging::start()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = true;
    }
    cond_.notify_all();
    thread_.start();
}

void Asynclogging::append(const char *msg, size_t len)
{

    std::lock_guard<std::mutex> lock(mutex_);
    if (currentBuffer_->avail() > len)
    {
        // LOG_INFO("日志写入buffer了\n");
        currentBuffer_->append(msg, len);
    }
    else
    {

        buffers_.push_back(std::move(currentBuffer_));

        if (nextBuffer_)
        {

            currentBuffer_ = std::move(nextBuffer_);
        }
        else
        {

            currentBuffer_.reset(new LogBuffer());
        }
        currentBuffer_->append(msg, len);
        cond_.notify_one();
    }
}

void Asynclogging::threadFunc()
{

    BufferPtr newBuffer1(new LogBuffer);
    BufferPtr newBuffer2(new LogBuffer);
    newBuffer1->reset();
    newBuffer2->reset();

    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);

    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]
                   {
                       return running_.load(std::memory_order_acquire); // 显式加载原子值
                   });
    }

    while (running_)
    {

        {
            std::unique_lock<std::mutex> lock(mutex_);

            if (buffers_.empty())
            {
                cond_.wait_for(lock, std::chrono::seconds(flushInterval_));
            }
            // LOG_INFO("准备将curruntbuffer送进去写文件了\n");
            buffers_.push_back(std::move(currentBuffer_));
            currentBuffer_ = std::move(newBuffer1);
            buffersToWrite.swap(buffers_);
            if (!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        if (buffersToWrite.size() > 2)
        {
            char buf[256];
            snprintf(buf, sizeof(buf), "Dropped %zd logs\n", buffersToWrite.size() - 2);
            fout_ << buf;
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
        }

        std::string bulk;
        size_t totalLen = 0;
        for (auto &buf : buffersToWrite)
        {
            totalLen += buf->length();
        }
        bulk.reserve(totalLen);

        for (auto &buf : buffersToWrite)
        {
            bulk.append(buf->data(), buf->length());
        }
        flushToFile(bulk); // 改为批量写入

        if (!newBuffer1 && !buffersToWrite.empty())
        {
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if (!newBuffer2 && !buffersToWrite.empty())
        {
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        fout_.flush();
    }
}

void Asynclogging::flushToFile(const std::string &buf)
{
    // 实现文件写入逻辑（可添加日志滚动逻辑）
    fout_.write(buf.data(), buf.size()); // 更高效的写入方式
}

void Asynclogging::open(const std::string &filename)
{

    std::lock_guard<std::mutex> lock(mutex_);
    filename_ = filename;
    fout_.open(filename, std::ios::trunc);

    if (fout_.fail())
    {

        throw std::logic_error("Failed to open file: " + filename);
    }
}