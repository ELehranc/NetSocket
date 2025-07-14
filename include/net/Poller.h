#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

// muduo库中多路事件分发器的核心IO复用模块
class Poller : noncopyable
{

private:
    EventLoop *onwerLoop_; // 定义Poller所属的事件循环

protected:
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;

public:
    using ChannelList = std::vector<Channel *>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    // 给所有IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    // 判断参数channel是否在当前poller下
    bool hasChannel(Channel *channel) const;

    // EventLoop可以通过这个接口获得一个具体的poller实现
    static Poller *newDefaultPoller(EventLoop *loop);
};
