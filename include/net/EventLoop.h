#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "Poller.h"
#include "CurrentThread.h"
class Channel;
class EpollPoller;
// 事件循环类， 主要包含了两大类， channel 和 poller
// 对应的就是事件反应器 Reactor组件
class EventLoop : noncopyable
{

public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // 在当前loop中执行cb
    void runInLoop(Functor cb);
    // 把cb放入任务队列，唤醒相应线程的loop执行cb
    void queueInLoop(Functor cb);

    // 唤醒loop所在线程
    void wakeup();

    // EventLoop的方法，调用Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

private:  
    void handleRead();        // 处理唤醒事件
    void handleTimer();       // 处理定时任务
    void doPendingFunctors(); // 执行回调

    using ChannelList = std::vector<Channel *>;


    std::atomic_bool looping_;
    std::atomic_bool quit_; // 标志退出loop循环

    const pid_t threadId_; // 记录当前loop所在的线程的id

    Timestamp pollReturnTime_; // poller返回发生事件的channels的时间点

    std::unique_ptr<Poller> poller_;

    int wakeupfd_; // 主要作用，当mainloop获取一个新用户的channel，通过轮询算法选择一个subloop，通过wakeupfd通知
    int timerfd_;
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;

    std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有需要执行的回调操作
    std::mutex mutex_;                        // 互斥锁，保护下面vector容器的线程安全操作
    std::vector<Functor> pendingFunctors_;    // 存储当前loop需要执行的所有的回调操作
};
