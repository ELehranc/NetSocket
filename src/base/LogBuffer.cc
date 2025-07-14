#include "LogBuffer.h"

LogBuffer::LogBuffer() : cur_(buffer_)
{
    memset(buffer_, 0, sizeof(buffer_));
}

size_t LogBuffer::avail() const
{
    return static_cast<int>(end() - cur_);
}

void LogBuffer::append(const char *msg, size_t len)
{
    memcpy(cur_, msg, len);
    cur_ += len;
}

const char *LogBuffer::end() const
{
    return buffer_ + kBufferSize; // 
}

const char *LogBuffer::data() const
{
    return buffer_;
}
size_t LogBuffer::length() const
{
    return cur_ - buffer_;
}

void LogBuffer::reset()
{
    cur_ = buffer_;
    memset(buffer_, 0, sizeof(buffer_));
}