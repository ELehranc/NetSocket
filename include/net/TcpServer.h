#pragma once
#include "noncopyable.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Acceptor.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "Connection.h"
#include "AsyncLog.h"

#include <functional>
#include <memory>
#include <atomic>
#include <unordered_map>

class TcpServer : noncopyable
{

public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    using ConnectionMap = std::unordered_map<std::string, ConnectionPtr>;

    enum Option
    {
        kNoReusePort,
        kReusePort
    };

    TcpServer(EventLoop *loop, const InetAddress &ListenAddr, const std::string &name, Option option = kNoReusePort);
    ~TcpServer();

    void setThreadNum(int numThreads);

    void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }

    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    // 开启服务器监听
    void start();
    EventLoop *getLoop() const { return loop_; }
    std::string getName() const { return name_; }
    std::string getIpPort() const { return ipPort_; }

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const ConnectionPtr &);
    void removeConnectionInLoop(const ConnectionPtr &);

    EventLoop *loop_; // baseloop,用户自己定义的loop
    std::mutex mutex_;
    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_; // 运行在mainloo，监听新的连接

    std::shared_ptr<EventLoopThreadPool> threadpool_; // one loop per thread

    // 供业务类进行设置
    ConnectionCallback connectionCallback_;       // 有新连接时的回调
    MessageCallback messageCallback_;             // 有新消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完全时的回调

    ThreadInitCallback threadInitCallback_;

    std::atomic_int started_;
    int nextConnId_;
    ConnectionMap connections_; // 保存所有的连接
};