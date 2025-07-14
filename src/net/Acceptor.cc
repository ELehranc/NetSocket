#include "Acceptor.h"
#include "Logger.h"
#include "AsyncLog.h"
#include <unistd.h>

static int createNonblocking()
{

    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {

        // LOG_FATAL("acceptsockfd creat failer:%d \n", sockfd);
        LOG(AsyncLog::FATAL, "acceptsockfd creat failer:%d \n", sockfd);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuesport)
    : loop_(loop), acceptsockfd_(createNonblocking()), acceptChannel_(loop_, acceptsockfd_.fd()), listenning_(false)
{
    acceptsockfd_.setKeepAlive(true);
    acceptsockfd_.setReuseAddr(true);
    acceptsockfd_.setReusePort(true);
    acceptsockfd_.setTcpNoDelay(true);

    acceptsockfd_.bindAddress(listenAddr);
    // mainloop => acceptchannle => Channel::readcallback => Acceptor::handleread
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}
Acceptor ::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}
void Acceptor ::listen()
{
    listenning_ = true;
    acceptsockfd_.listen();
    acceptChannel_.enableReading();
}
// accpetfd上有新的连接到达，但是还没有响应，你应该去主动的进行接收，你才能获得他的fd
void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = acceptsockfd_.accept(&peerAddr);

    if (connfd > 0)
    {
        if (newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peerAddr);   // 轮询找subloop，唤醒从循环，分发新客户端channel
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
}