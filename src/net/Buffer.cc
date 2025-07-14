#include "Buffer.h"
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

const char Buffer::kCRLF[] = "\r\n";

// 从fd上直接读取数据，pooller在lt模式
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char extrabuf[65536] = {0}; // 栈上内存空间

    iovec vec[2];
    const size_t writable = writableBytes(); // 这是整个Buffer底层缓冲区剩余的可写空间大小

    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovecnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovecnt);

    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable)
    {
        writerIndex_ += n; // 如果是这个情况，数据是一家直接通过readv放入了buffer_中，这里只需要对index进行操作就可以了
    }
    else
    {
        // extrabuffer中存有了部分数据,这个时候需要将extrabuffer里的数据放到buffer_中
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable); // 这一段就是将extrbuff中长度为n-writable的数据放到buffer_中，其中会考虑扩容或者移位
    }

    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno)
{

    ssize_t n = ::write(fd, peek(), readableBytes());

    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}