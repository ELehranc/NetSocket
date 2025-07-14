#pragma once

#include <vector>
#include <string>
#include <algorithm>

/*
        |  kCheapPrepend |    readbleBytes |   writableBytes    |
                         o                 o
                    readindex          writeindex
*/

class Buffer
{

private:
    char *begin() { return &*buffer_.begin(); }             // vector底层数组首元素的地址，也就是数组的起始地址
    const char *begin() const { return &*buffer_.begin(); } // vector底层数组首元素的地址，也就是数组的起始地址

    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

    static const char kCRLF[];

public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize) : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend),
                                   writerIndex_(kCheapPrepend)
    {
    }

    size_t readableBytes()
    {
        return writerIndex_ - readerIndex_;
    }
    size_t writableBytes()
    {
        return buffer_.size() - writerIndex_;
    }
    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    const char *peek() const
    {
        return Buffer::begin() + readerIndex_; // 返回可读区域的第一个数据的地址
    }

    // onMessage string <- Buffer
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {

            readerIndex_ += len; // 应用只读取了可读缓冲区的len长度的部分，还有writerIndex - len的内容可读
        }
        else
        {

            retrieveAll();
        }
    }

    void retrieveUntil(const char *end)
    {
        retrieve(end - peek());
    }

    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len); // 上面一句把缓冲区中可读的数据，已经读取出来了，需要进行复位
        return result;
    }

    // buffer_.size -writerIndex_  ::  len
    void ensureWriteableByte(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
    }

    // 把[data,data+len]的数据添加到writabele缓冲区中
    void append(const char *data, size_t len)
    {

        ensureWriteableByte(len);
        std::copy(data, data + len, beginwrite());
        writerIndex_ += len;
    }

    char *beginwrite()
    {
        return begin() + writerIndex_;
    }

    const char *beginwrite() const
    {
        return begin() + writerIndex_;
    }

    const char *findCRLF() const
    {
        const char *crlf = std::search(peek(), beginwrite(), kCRLF, kCRLF + 2);
        return crlf == beginwrite() ? NULL : crlf;
    }

    // 从fd上直接读取数据
    ssize_t readFd(int fd, int *saveErrno);
    ssize_t writeFd(int fd, int *saveErrno);
};