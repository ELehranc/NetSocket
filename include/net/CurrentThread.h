// CurrentThread.h（正确写法）
#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    extern __thread int t_cachedTid;
    void cacheTid();
    inline int tid();
}

// 内联函数实现（可选，若头文件包含实现）
inline int CurrentThread::tid()
{
    if (__builtin_expect(t_cachedTid == 0, 0))
    {
        cacheTid();
    }
    return t_cachedTid;
}