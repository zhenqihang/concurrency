#pragma once

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>
#include <assert.h>

class ThreadPool{
public:
    explicit ThreadPool(size_t threadCount=8): pool_(std::make_shared<Pool>()) {
        assert(threadCount > 0);
        for(size_t i=0; i<threadCount; i++) {
            std::thread([pool = pool_] {
                std::unique_lock<std::mutex> locker(pool->mtx);  // 线程同步
                while (true)
                {
                    /* code */
                    if(!pool->tasks.empty()) {
                        auto task = std::move(pool->tasks.front());  // std::move 拷贝构造函数，堆分配
                        pool->tasks.pop();
                        // 解锁
                        locker.unlock();

                        task();

                        locker.lock();
                    }
                    else if(pool->isClosed) break;
                    else pool->cond.wait(locker);
                    }
                
            }).detach();  // 线程分离
        }
    }

    ThreadPool() = default;

    ThreadPool(ThreadPool&&) = default;

    ~ThreadPool()
    {
        if(static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->isClosed = true;
            }
            pool_->cond.notify_all();
        }
    }

    template<class F>
    void AddTask(F&& task)
    {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<F>(task));      // emplace优于insert
        }
        pool_->cond.notify_one();  // 唤醒一个线程
    }

private:
    struct Pool {
        std::mutex mtx;  // 互斥锁
        std::condition_variable cond;  // 条件变量
        bool isClosed;
        std::queue<std::function<void()>> tasks;  // 保存任务
    };
    std::shared_ptr<Pool> pool_;  // 线程池
};
