#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>
#include <iostream>
std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false),
      joined_(false),
      tid_(0),
      threadfunc_(std::move(func)),
      name_(name)
{
    setDefaultName();
}
Thread::~Thread()
{
    if (started_ && !joined_)
    {
        thread_->detach(); // thread类提供的线程分离函数
    }
}
void Thread::start() // 一个Thread对象，记录的就是一个新线程的详细信息
{
    started_ = true;

    sem_t sem;
    sem_init(&sem, false, 0);

    thread_ = std::shared_ptr<std::thread>(new std::thread([&]()
    { 
        tid_ = CurrentThread::tid();
        sem_post(&sem);

        threadfunc_(); }));

    // 这里必须等待获取上面新创建的线程的tid值
    sem_wait(&sem);
}
void Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d \n", num);
        name_ = buf;
    }
}
