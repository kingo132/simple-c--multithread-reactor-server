#ifdef __linux__
#include "epoll_dispatcher.h"
#include "log_manager.h"

EpollDispatcher::EpollDispatcher() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        LOG_ERR("Failed to create epoll instance");
        exit(EXIT_FAILURE);
    }
}

EpollDispatcher::~EpollDispatcher() {
    close(epoll_fd_);
}

void EpollDispatcher::add_fd(int fd) {
    epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = fd;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) == -1) {
        LOG_ERR("Failed to add file descriptor to epoll");
    }
}

void EpollDispatcher::remove_fd(int fd) {
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
}

void EpollDispatcher::wait_and_handle_events(int timeout_milliseconds, const std::function<void(int fd, bool is_readable)>& handler) {
    int num_events = epoll_wait(epoll_fd_, events_, MAX_EVENTS, timeout_milliseconds);
    for (int i = 0; i < num_events; ++i) {
        int fd = events_[i].data.fd;
        bool is_readable = (events_[i].events & EPOLLIN) != 0;
        handler(fd, is_readable);
    }
}
#endif // __linux__
