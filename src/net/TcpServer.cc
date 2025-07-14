#include "TcpServer.h"
#include "Logger.h"
#include <string.h>

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{

    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

/*

*/

TcpServer::TcpServer(EventLoop *loop, const InetAddress &ListenAddr, const std::string &name, Option option)
    : loop_(CheckLoopNotNull(loop)),
      ipPort_(ListenAddr.toIpPort()),
      name_(name),
      acceptor_(new Acceptor(loop_, ListenAddr, option == kNoReusePort)),
      threadpool_(new EventLoopThreadPool(loop_, name_)),
      connectionCallback_(),
      messageCallback_(),
      nextConnId_(1),
      started_(0)

{
    // channel::readCallback => Acceptor::handleRead => TcpServer::newConnection
    acceptor_->setNewConnecitonCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}
TcpServer::~TcpServer()
{

    for (auto &item : connections_)
    {
        ConnectionPtr conn(item.second);
        item.second.reset();

        conn->getLoop()->runInLoop(std::bind(&Connection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    threadpool_->setThreadNum(numThreads);
}

// 开启服务器监听
void TcpServer::start()
{
    if (started_++ == 0)
    {
        threadpool_->start(threadInitCallback_);                         // 按照initCallback开启全部线程池中的线程
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get())); // 开启主线程的监听
    }
}


void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    EventLoop *ioLoop = threadpool_->getNextLoop(); // 获得一个Io线程
    char buf[64];
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;

    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n", name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    sockaddr_in local;
    ::bzero(&local, sizeof(local));
    socklen_t addrlen = sizeof(local);
    if (::getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr \n");
    }

    InetAddress localAddr(local);

    // 根据连接成功得sockfd，创建Connection连接对象
    ConnectionPtr conn(new Connection(ioLoop, connName, sockfd, localAddr, peerAddr));

    connections_[connName] = conn;
    // 下面得回调都是用户设置给TcpServer =>Connection => Channel ,Channel被Poller通知调用handle函数，回调相应设置得Connection => TcpServer => 业务
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    // 直接调用conneciton得连接建立函数
    ioLoop->runInLoop(std::bind(&Connection::connectEstablished, conn));
}
void TcpServer::removeConnection(const ConnectionPtr &conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}
void TcpServer::removeConnectionInLoop(const ConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n", name_.c_str(), conn->name().c_str());

    connections_.erase(conn->name());
    EventLoop *ioloop = conn->getLoop();
    ioloop->queueInLoop(std::bind(&Connection::connectDestroyed, conn));
}
