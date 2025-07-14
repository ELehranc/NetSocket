#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include <memory>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseloop, const std::string &nameArg)
    : baseLoop_(baseloop),
      name_(nameArg),
      started_(false),
      numThreads_(0),
      next_(0)
{
}
EventLoopThreadPool ::~EventLoopThreadPool()
{
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb )
{

    started_ = true;

    for (int i = 0; i < numThreads_; i++)
    {
        char buf[32 + name_.size()];
        snprintf(buf, sizeof(buf), "%s%d", name_.c_str(), i);

        EventLoopThread *t = new EventLoopThread(cb, buf);
        // EventLoopThread对象，没有调用startloop，不会创建真正的线程
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        

        loops_.push_back(t->startloop()); // 真正底层创建并启动新的线程，并返回了绑定的EventLoop地址
    }

    // 如果服务端只有一个线程，运行着baseloop
    if (numThreads_ == 0 && cb)
    {

        cb(baseLoop_);
    }
}

// 默认采取轮询的方式分配channel给subloop
EventLoop *EventLoopThreadPool::getNextLoop()
{

    EventLoop *loop = baseLoop_;

    // 通过轮询来获得工作线程，否则使用主循环
    if (!loops_.empty())
    {
        loop = loops_[next_];
        next_++;
        if (next_ >= loops_.size())
        {
            next_ = 0;
        }
    }

    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllloops()
{
    if (loops_.empty())
    {

        return std::vector<EventLoop *>(1, baseLoop_);
    }
    else
    {

        return loops_;
    }
}