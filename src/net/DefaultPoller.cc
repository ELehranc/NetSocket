#include "Poller.h"
#include "Epollpoller.h"
#include <stdlib.h>

Poller *Poller::newDefaultPoller(EventLoop *loop)
{

    if (::getenv("MUDUO_USE_POLL"))
    {

        return nullptr; // 生成Poll实例
    }
    else
    {

        return new Epollpoller(loop); // 生成Epoll实例
    }
}