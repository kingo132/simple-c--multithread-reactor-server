#ifdef __linux__
#ifndef EPOLL_DISPATCHER_H
#define EPOLL_DISPATCHER_H

#include "event_dispatcher.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <functional>

class EpollDispatcher : public EventDispatcher {
public:
    EpollDispatcher();
    ~EpollDispatcher();

    void add_fd(int fd) override;
    void remove_fd(int fd) override;
    void wait_and_handle_events(int timeout_milliseconds, const std::function<void(int fd, bool is_readable)>& handler) override;

private:
    int epoll_fd_;
    static const int MAX_EVENTS = 1024;
    epoll_event events_[MAX_EVENTS];
};

#endif // EPOLL_DISPATCHER_H
#endif // __linux__
