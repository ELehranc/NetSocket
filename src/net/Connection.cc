#include "Connection.h"
#include "Logger.h"
#include "AsyncLog2.h"

#include <error.h>
#include <string>
static EventLoop *CheckLoopNotNull(EventLoop *loop)
{

    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d Concection Loop is null \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

Connection::Connection(EventLoop *loop, const std::string name, int sockfd, const InetAddress &LocalAddr, const InetAddress &peerAddr)
    : loop_(CheckLoopNotNull(loop)),
      name_(name),
      state_(kConnecting),
      sockfd_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(LocalAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024)
{
    // 下面给channel 设置相应的回调函数
    channel_->setReadCallback(std::bind(&Connection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&Connection::handleWrite, this));
    channel_->setErrorCallback(std::bind(&Connection::handleError, this));
    channel_->setCloseCallback(std::bind(&Connection::handleClose, this));
    LOG_INFO("Connection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);

    sockfd_->setKeepAlive(true);
}

Connection::~Connection()
{
    LOG_INFO("Connection::dtor[%s] at fd=%d state=%d \n", name_.c_str(), channel_->fd(), state_.load());
}

void Connection::send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->runInLoop(std::bind(&Connection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

void Connection::send(const char *data, const int len)
{
    send(std::string(data, len));
}

void Connection::sendInLoop(const void *data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing! \n");
        return;
    }

    if (!channel_->isWriting() && OutputBuffer_.readableBytes() == 0)
    {

        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                loop_->queueInLoop(std::bind(Connection::writeCompleteCallback_, shared_from_this()));
            }
        }
        else
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("Connection::sendInLoop \n");
                if (errno == EPIPE || errno == ECONNRESET)
                {

                    faultError = true;
                }
            }
        }
    }
    // 说明当前这一次发送，并没有直接把所有的数据都发送出去，剩余的数据需要保存到缓冲区当中，
    // 然后给channel注册epollout事件，等待可写事件发生进行回调处理
    if (!faultError && remaining > 0)
    {
        // 目前发送缓冲区剩余的带发送数据的长度
        size_t oldLen = OutputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(Connection::highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }

        OutputBuffer_.append((char *)data + nwrote, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting(); // 启动channel的写事件
        }
    }
}

void Connection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&Connection::shutdownInloop, this));
    }
}

void Connection::shutdownInloop()
{
    if (!channel_->isWriting()) // 说明当前outputbuffer中的数据已经发送完成了
    {
        sockfd_->shutdonwWrite(); // 关闭fd的写端,则会引起channel的closehandle回调
    }
}

void Connection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    // 现在有一个新的连接建立，而且也将channel放入到poller中了
    connectionCallback_(shared_from_this());
}
// 真正执行删除连接得函数 最终回调它
void Connection::connectDestroyed()
{

    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); // 把channel下树
        connectionCallback_(shared_from_this());
    }
    channel_->remove(); // 从poller的容器中删除
}

void Connection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = InputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        // 已建立的用户发来的数据，造成了channel的回调函数，目前到达了Conneciton层

        messageCallback_(shared_from_this(), &InputBuffer_, receiveTime);
    }
    else if (n == 0)
    {

        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_DEBUG("Connection::handleRead error \n");
        handleError();
    }
}
void Connection::handleWrite()
{
    int savedErrno = 0;
    if (channel_->isWriting())
    {
        int saveErrno = 0;
        ssize_t n = OutputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if (n > 0)
        {
            OutputBuffer_.retrieve(n);
            if (OutputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }

                if (state_ == kDisconnecting)
                {
                    shutdownInloop();
                }
            }
        }
        else
        {
            LOG_ERROR("Connection handleWrite \n");
        }
    }
    else
    {

        LOG_ERROR("Connection fd = %d is down , no more writing \n", channel_->fd());
    }
}

// poller => channel::closeCallback => Conncection::handleClose => TcpServer::removeConnection
void Connection::handleClose()
{
    LOG_INFO("fd=%d state=%d \n", channel_->fd(), state_.load());
    setState(kDisconnected);
    channel_->disableAll();

    ConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);
    closeCallback_(connPtr); // TcpServer::removeConnection
}
void Connection::handleError()
{

    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("Connection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}
