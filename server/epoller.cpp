
#include "epoller.h"

Epoller::Epoller(int maxEvent):epollFd_(epoll_create(512)), events_(maxEvent)
{
    assert(epollFd_>=0 && events_.size()>0);
}

Epoller::~Epoller()
{
    close(epollFd_);
}

bool Epoller::AddFd(int fd, uint32_t events)
{
    if(fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;

    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::ModFd(int fd, uint32_t events)
{
    if(fd<0) return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;

    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);  // 返回0表示成功，否则返回–1
}

bool Epoller::DelFd(int fd)
{
    if(fd<0) return false;
    epoll_event ev = {0};
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev);  // 返回0表示成功，否则返回–1
}

// 返回等待事件个数
int Epoller::wait(int timeoutMs)
{
    return epoll_wait(epollFd_, &events_[0], static_cast<int>(events_.size()), timeoutMs);  // static_cast强制类型转换
}

int Epoller::GetEventFd(size_t i) const
{
    assert(i < events_.size() && i>=0);
    return events_[i].data.fd;
}

uint32_t Epoller::GetEvents(size_t i) const
{
    assert(i < events_.size() && i>=0);
    return events_[i].events;
}