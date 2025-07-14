#include "Channel.h"
#include "Logger.h"
#include "EventLoop.h"
#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd) : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}
Channel::~Channel()
{
}

// 防止当channel被手动remove调，channel还在执行回调函数
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

/*
    负责在poller中修改fd相关的事件，epoll_ctl;
    但是，这件事情不是channel做的，是需要poller做的，
    所以只能通过loop_这个桥梁来进行联系,调用poller的相关的函数
*/
void Channel::update()
{
    loop_->updateChannel(this);
}

// 在channel所属的loop中， 将当前channel删除掉
void Channel::remove()
{
    loop_->removeChannel(this);
}

// fd得到poller通知以后，处理事件的函数，里面根据不同的情况调用不同的回调函数
void Channel::handleEvent(Timestamp receiveTime)
{
    if (tied_)
    {

        std::shared_ptr<void> guard = tie_.lock();

        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}
// 由poller通知发生了具体时间的channel，channel在这个函数下再自己判断并调用相应的回调函数进行处理
void Channel::handleEventWithGuard(Timestamp receiveTime)
{

    // LOG_INFO("channel handleEvent revents: %d \n", revents_);

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallback_)
        {
            closeCallback_();
        }
    }

    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }

    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
    }

    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
}

void Channel::enableReading()
{
    events_ |= kReadEvent;
    update();
}
void Channel::disableReading()
{
    events_ &= ~kReadEvent;
    update();
}
void Channel::enableWriting()
{
    events_ |= kWriteEvent;
    update();
}
void Channel::disableWriting()
{
    events_ &= ~kWriteEvent;
    update();
}
void Channel::disableAll()
{
    events_ = kNoneEvent;
    update();
}
