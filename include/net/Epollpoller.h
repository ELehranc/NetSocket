#pragma once

#include "Poller.h"

#include <vector>
#include <sys/epoll.h>
#include "Timestamp.h"

class Channel;

class Epollpoller : public Poller
{

private:
    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_;

    static const int kInitEventListSize = 16;
    
    // 填写epoll_wait返回的活跃Channel
    void fillActiveChannels(int num,ChannelList *activeChannels) const;
    // 更新Channel上的感兴趣的事件
    void update(int operation, Channel *channel);           // 真正底层实现epoll_ctl
public: 
    Epollpoller(EventLoop *loop);                          // 真正底层实现epoll_create 
    ~Epollpoller() override;

    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;          // 封装update
    void removeChannel(Channel *channel) override;          // 封装update
};
