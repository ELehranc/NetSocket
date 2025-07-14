#pragma once
#include <cstddef>
#include <string.h>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "Thread.h"

class LogBuffer
{

public:
    static const size_t kBufferSize = 4096;

    LogBuffer();
    void append(const char *msg, size_t len);

    const char *data() const;
    size_t length() const;
    size_t avail() const;
    void reset();

private:
    const char *end() const;

    char buffer_[kBufferSize];
    char *cur_;
};