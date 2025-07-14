#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include <functional>

class Acceptor : noncopyable
{

public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;

    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuesport);
    ~Acceptor();

    void setNewConnecitonCallback(const NewConnectionCallback &cb) { newConnectionCallback_ = cb; }

    bool lisntenning() const { return listenning_; }
    void listen();

private:
    void handleRead();
    EventLoop *loop_;       // 用户定义的mainloop
    Socket acceptsockfd_;   // TcpSevrer的监听sockfd
    Channel acceptChannel_; // 封装了sockfd和其上所关心的事件，也就是接收事件

    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};
