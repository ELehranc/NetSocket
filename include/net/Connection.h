#pragma once
#include "noncopyable.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <memory>
#include <string>
#include <atomic>
#include <functional>
//  Connection 封装的是一条和客户端明确建立了连接的链路
//  这个部分的大部分是已经由Channel承担了功能，但是Channel只是实际的将fd和事件联系在了一起，其他的处理逻辑是依靠了callback_;
//  很明显，这些callback其实就应该从Connection中传递过来
//  那多想一步，Connection的Callback来自谁呢，来自TcpServer

/*
    TcpServer => Acceptor => 有一个新的用户连接 =>  handleRead 获得connfd => 回调TcpServer => 分给subloop =>

*/

class Connection : noncopyable, public std::enable_shared_from_this<Connection>
{
private:
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void *message, size_t len);
    void shutdownInloop();

    enum StateE
    {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting
    };
    EventLoop *loop_; // 这里是从事间循环，从mainloop分发过来的客户端fd构成了conn。
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> sockfd_;
    std::unique_ptr<Channel> channel_;

    std::string context_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    // 供TcpServer进行设置
    ConnectionCallback connectionCallback_;       // 有新连接时的回调
    MessageCallback messageCallback_;             // 有新消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完全时的回调
    HighWaterMarkCallback highWaterMarkCallback_; // 高水位的回调
    CloseCallback closeCallback_;                 // 连接断开时的回调

    size_t highWaterMark_;

    Buffer InputBuffer_;
    Buffer OutputBuffer_;
public:
    Connection(EventLoop *loop, const std::string name, int sockfd, const InetAddress &LocalAddr, const InetAddress &peerAddr);

    ~Connection();

    EventLoop *getLoop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAddress &LocalAddr() const { return localAddr_; }
    const InetAddress &peerAddr() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; }

    void send(const std::string &buf);
    void send(const char *data, const int len);
    void shutdown();

    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb) { highWaterMarkCallback_ = cb; }
    void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }

    void connectEstablished();
    void connectDestroyed();

    void setState(StateE state) { state_ = state; }
};