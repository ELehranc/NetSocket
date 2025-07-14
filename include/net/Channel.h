#pragma once

#include <functional>
#include <memory>

#include "noncopyable.h"
#include "Timestamp.h"
/*
    Channel 理解为通道，封装了socketfd和这个fd上所感兴趣的事件和发生的具体事件
    EeventLoop 包含了 Channel 和 Poller， 他们对应了模型上的 Demultiplex 事件多路分发器

*/
class EventLoop;
class Timestamp;

class Channel
{

private:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // 事件循环
    const int fd_;    // fd ， Poller监听的对象
    int events_;      // 所关心的事件
    int revents_;     // Poller真正返回的具体发生的事件
    int index_;       // 所属的Poller的编号

    std::weak_ptr<void> tie_;
    bool tied_;

    // 因为channel通道里面获知fd最终发生的具体事件revents，所以它负责条用具体时间的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

    void update();
    void handleEventWithGuard(Timestamp receiveTime);

public:
    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到poller通知以后，处理事件的函数，里面根据不同的情况调用不同的回调函数
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止当channel被手动remove调，channel还在执行回调函数
    void tie(const std::shared_ptr<void> &);

    int fd() const { return fd_; }
    int events() const { return events_; }
    int set_revents(int revt) { return revents_ = revt; } // 由poller进行设置

    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    void enableReading();
    void disableReading();
    void enableWriting();
    void disableWriting();
    void disableAll();

    int index() { return index_; }
    void set_index(int idex) { index_ = idex; }

    // one loop per thread
    EventLoop *OwnerLoop() { return loop_; }

    void remove();
};