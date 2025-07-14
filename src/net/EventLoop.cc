#include "EventLoop.h"
#include "Logger.h"
#include "Epollpoller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <cstring>
#include <sys/timerfd.h>
#include <unistd.h>
#include <memory>

// 防止一个线程创建多个EventLoop
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用的超时时间
const int kPollTimeMs = 10000;
// 创建wakeupfd，用来notify subReactor来接收新的连接或者处理任务
int creatEventfd()
{

    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {

        LOG_FATAL("eventfd CREATE error:%d\n", errno);
    }
    return evtfd;
}

int createTimerfd(int sec)
{

    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    struct itimerspec timeout;
    memset(&timeout, 0, sizeof(struct itimerspec));
    timeout.it_value.tv_sec = sec;
    timeout.it_value.tv_nsec = 0;
    timerfd_settime(tfd, 0,&timeout, 0);
    return tfd;
}

EventLoop::EventLoop() : looping_(false),
                         quit_(false),
                         callingPendingFunctors_(false),
                         threadId_(CurrentThread::tid()),
                         poller_(Poller::newDefaultPoller(this)),
                         wakeupfd_(creatEventfd()),
                         wakeupChannel_(new Channel(this, wakeupfd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型以及之后要做的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个subloop都会监听这个wakechannel的读事件，以便mainloop对各个subloop的通知和唤醒
    wakeupChannel_->enableReading();
}
EventLoop::~EventLoop()
{
    wakeupChannel_->remove(); // 里面包含了对事件的置位，以及调用update函数
    ::close(wakeupfd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::handleRead()
{

    uint64_t one = 1;
    ssize_t n = read(wakeupfd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::handleRead() reads % bytes instead of 8 \n", n);
    }
}

void EventLoop::loop()
{

    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);
    while (!quit_)
    {

        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);

        for (auto channel : activeChannels_)
        {
            // Poller 上报上来的活跃通道，进行具体事务的处理
            channel->handleEvent(pollReturnTime_);
        }

        /*
           接下来，subloop需要执行当前EventLoop事件循环需要处理的回调操作，
           因为一个subloop监听被唤醒，有可能是channel的信号到了，这一点，在上面channel已经通过handleEvent处理掉了
           还有可能，就是别的事件唤醒了subloop，需要执行下面的方法,比方说，一个新的客户链接到达，你需要做一些操作，
           这个是不会被上面的循环给照顾到的。

           而在第一版的程序中，其实是在Tcp的回调函数中，就帮subloop完成这一点了，通过Connection对象的创建，就已经完成了对
           channel的注册、绑定loop等等一系列操作
        */
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping \n", this);
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;

    if (!isInLoopThread())
    {

        wakeup();
    }
}
// 在当前loop中执行cb
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}
// 把cb放入任务队列，唤醒相应线程的loop执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> gd(mutex_);

        pendingFunctors_.emplace_back(cb);
    }

    // 唤醒相应的，需要执行上面回调的loop的线程了
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup(); // 唤醒所在loop线程
    }
}
// 唤醒loop所在线程
// 向wakeupfd_写一个数据就行
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupfd_, &one, sizeof(one));

    if (n != sizeof(n))
    {

        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

// EventLoop的方法，调用Poller的方法
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::unique_lock<std::mutex> gd(mutex_);
        functors.swap(pendingFunctors_);
    }
    for (const Functor &functor : functors)
    {
        functor();
    }
    callingPendingFunctors_ = false;

} // 执行回调
