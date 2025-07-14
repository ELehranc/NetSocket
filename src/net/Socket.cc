#include "Socket.h"
#include "Logger.h"

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>

Socket ::~Socket()
{
    close(sockfd_);
}

void Socket::bindAddress(const InetAddress &Localaddr)
{
    if (bind(sockfd_, (sockaddr *)Localaddr.getSockAddr(), sizeof(sockaddr_in)) != 0)
    {

        LOG_FATAL("bind sockfd:%d fail %d\n", sockfd_, errno);
    }
}
void Socket::listen()
{

    if (::listen(sockfd_, 5) != 0)
    {
        LOG_FATAL("listen sockfd:%d fail \n", sockfd_);
    }
}
int Socket::accept(InetAddress *peeraddr)
{
    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    socklen_t len = sizeof(addr);

    int connfd = ::accept4(sockfd_, (sockaddr *)&addr, &len,SOCK_NONBLOCK|SOCK_CLOEXEC);

    if (connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdonwWrite()
{
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("shudownWrite error \n");
    }
}
void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}