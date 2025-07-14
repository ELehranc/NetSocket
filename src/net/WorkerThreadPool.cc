#include "WorkerThreadPool.h"
#include "Logger.h"

WorkThreadPool::WorkThreadPool(size_t threadNum) : stop_(false)
{

    for (size_t i = 0; i < threadNum; i++)
    {

        threads_.emplace_back([this]
                              { LOG_INFO("create %s thread(%d).\n", "workthread", syscall(SYS_gettid));
                          while(stop_ == false){
                            std::function<void()> task;
                            {
                                std::unique_lock<std::mutex> lock(mutex_);
                                condition_.wait(lock, [this]
                                                { return ((stop_ == true) || taskqueue_.empty() == false); });

                                if((stop_ == true) && (taskqueue_.empty()== true))
                                {
                                    return;
                                }
                                task = std::move(taskqueue_.front());
                                taskqueue_.pop();
                            }
                                printf("%s(%d) execute task.\n", "workthread", syscall(SYS_gettid));
                                task();
                            
                            } });
    }
}

void WorkThreadPool::addtask(std::function<void()> task)
{

    {
        std::lock_guard<std::mutex> lock(mutex_);
        taskqueue_.push(task);
    }
    condition_.notify_one();
}

WorkThreadPool::~WorkThreadPool()
{
    stop_ = true;

    condition_.notify_all(); // 唤醒全部的线程。

    // 等待全部线程执行完任务后退出。
    for (std::thread &th : threads_)
        th.join();
}

// 获取线程池的大小。
size_t WorkThreadPool::size()
{
    return threads_.size();
}