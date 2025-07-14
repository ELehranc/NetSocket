#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name) : loop_(nullptr), exiting_(false), thread_(std::bind(&EventLoopThread::threadFunc, this), name), mutex_(), cond_(), callback_(cb)
{
}
EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::startloop()
{
    thread_.start(); // 启动底层的新线程的绑定函数

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> gd(mutex_);
        while (loop_ == nullptr)
        {
            cond_.wait(gd);
        }
        loop = loop_;
    }
    return loop;
}

// 下面这个方法，是在单独的新线程里面运行的
void EventLoopThread::threadFunc()
{
    EventLoop loop; // 创建一个独立的eventloop，和上面的线程是一一对应的

    if (callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> gd(mutex_);
        loop_ = &loop; // 将创建的loop赋值给对象进行绑定
        cond_.notify_one();
    }

    loop.loop();
    std::unique_lock<std::mutex> gd(mutex_);
    loop_ = nullptr;
}