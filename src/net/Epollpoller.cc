#include "Epollpoller.h"
#include "Logger.h"
#include "unistd.h"
#include "Channel.h"
#include <string.h>

const int kNew = -1; // Channel index成员的含义，用于表示Channel在poller中的状态
const int kAdded = 1;
const int kDeleted = 2;

Epollpoller::Epollpoller(EventLoop *loop) : Poller(loop), epollfd_(epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error %d \n", errno);
    }
} // 真正底层实现epoll_create
Epollpoller::~Epollpoller()
{
    ::close(epollfd_);
}

// EventLoop 通过指定activechannel的容器，让poll将数据填写进入，从而得到就绪通道
Timestamp Epollpoller::poll(int timeoutMs, ChannelList *activeChannels)
{

    LOG_DEBUG("func=%s => fd  count:%d\n", __FUNCTION__, channels_.size());
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if (numEvents > 0)
    {

        LOG_DEBUG("%d envents happend \n", numEvents);
        fillActiveChannels(numEvents, activeChannels); // 填写EventLoop传入的容器
        if (numEvents == events_.size())
        {
            events_.resize((events_.size()) * 2); // MUDUO库采取的是LT模式，所以没有存储进的事件下次仍会返回
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else
    {

        if (saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll error!\n");
        }
    }

    return now;
}
// 填写epoll_wait返回的活跃Channel
void Epollpoller::fillActiveChannels(int num, ChannelList *activeChannels) const
{
    for (int i = 0; i < num; i++)
    {

        Channel *ch = static_cast<Channel *>(events_[i].data.ptr);
        ch->set_revents(events_[i].events);

        activeChannels->push_back(ch); // 这个vector是EventLoop所传进来的一个容器
    }
}
void Epollpoller::removeChannel(Channel *channel)
{
    // LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd());
    int fd = channel->fd();
    channels_.erase(fd); // 从Poller的管理容器中删除channel
    channel->disableAll();

    int index = channel->index();
    if (index == kAdded)
    {
        updateChannel(channel); // 从Poller的监听树上删除channel
    }

} // 封装update

// channel update remove => EventLoop updateChannel removeChannel => Poller  updateChannel removeChannel
void Epollpoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    //LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), channel->index());

    if (index == kNew || index == kDeleted)
    {

        if (index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel; // 将channel添加入poller的管理容器
        }
        else
        {
        }
        // 如果channel是未添加或者之前添加但是已经删除过的，说明现在需要把他添加到当前poller中
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        int fd = channel->fd();
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }

} // 封装update
// 更新Channel上的感兴趣的事件
void Epollpoller::update(int operation, Channel *channel)
{
    epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = channel->events();
    ev.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, channel->fd(), &ev) < 0)
    {

        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }

} // 真正底层实现epoll_ctl