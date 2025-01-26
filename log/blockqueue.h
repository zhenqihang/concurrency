#pragma once

#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>
#include <assert.h>

template <class T>
class BlockDeque {
public:
    explicit BlockDeque(size_t MaxCapacity = 1000);

    ~BlockDeque();

    void clear();
    bool empty();
    bool full();
    void Close();
    size_t size();
    size_t capacity();
    T front();
    T back();

    void push_back(const T &item);
    void push_front(const T &item);
    bool pop(T &item);
    bool pop(T &item, int timeout);
    void flush();

private:
    std::deque<T> deq_;                      // 消息队列
    size_t capacity_;                        // 队列的大小
    std::mutex mtx_;                         // 互斥锁
    bool isClose_;                           // 
    std::condition_variable condConsumer_;   // 消费者，多线程的条件变量
    std::condition_variable condProducer_;   // 生产者，多线程的条件变量
};

template<class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity): capacity_(MaxCapacity) {
    assert(MaxCapacity>0);
    isClose_ = false;
}

template<class T>
BlockDeque<T>::~BlockDeque()
{
    Close();
}

template<class T>
void BlockDeque<T>::Close()
{
    {
        std::lock_guard<std::mutex> locker(mtx_);  // 在当前作用域内加锁，出作用域自动解锁
        deq_.clear();
        isClose_ = true;
    }
    condProducer_.notify_all();  // 唤醒所有线程
    condConsumer_.notify_all();  // 唤醒所有线程
}

template<class T>
void BlockDeque<T>::flush()
{
    condConsumer_.notify_one();  // 唤醒当前线程
}

template<class T>
void BlockDeque<T>::clear()
{
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
}

template<class T>
T BlockDeque<T>::front()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.front();
}

template<class T>
T BlockDeque<T>::back()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.back();
}

template<class T>
size_t BlockDeque<T>::size()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size();
}

template<class T>
size_t BlockDeque<T>::capacity()
{
    std::lock_guard<std::mutex> locker(mtx_);  // 作用域内加锁，轻锁
    return capacity_;
}

template<class T>
void BlockDeque<T>::push_back(const T &item)
{
    std::unique_lock<std::mutex> locker(mtx_);  // 这里不可使用lock_guard锁，因为wait方法需要在等待时解锁
    while(deq_.size()>=capacity_)   // 判断消息队列是否已满
    {
        condProducer_.wait(locker);  // 若消息队列已满，阻塞生产者
    }
    deq_.push_back(item);            // 添加 
    condConsumer_.notify_one();      // 唤醒消费者
}

template<class T>
void BlockDeque<T>::push_front(const T &item)
{
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.size()>=capacity_)
    {
        condProducer_.wait(locker);
    }
    deq_.push_front(item);
    condConsumer_.notify_one();
}

template<class T>
bool BlockDeque<T>::empty()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.empty();
}

template<class T>
bool BlockDeque<T>::full()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size() >= capacity_;
}

template<class T>
bool BlockDeque<T>::pop(T &item)
{
    std::unique_lock<std::mutex> locker(mtx_);  // 锁住当前线程
    while(deq_.empty())
    {
        condConsumer_.wait(locker);
        if(isClose_)
            return false;
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();

    return true;
}

template<class T>
bool BlockDeque<T>::pop(T &item, int timeout)
{
    std::unique_lock<std::mutex> locker(mtx_);  // 锁住当前线程
    while(deq_.empty())
    {
        if(condConsumer_.wait_for(locker, std::chrono::seconds(timeout))
                == std::cv_status::timeout) {
            return false;
        }
        if(isClose_)
            return false;
    }
    item.deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}