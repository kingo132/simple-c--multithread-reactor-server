#include "select_dispatcher.h"
#include "log_manager.h"

SelectDispatcher::SelectDispatcher() {
    FD_ZERO(&read_fds_);
    max_fd_ = -1;
}

void SelectDispatcher::add_fd(int fd) {
    FD_SET(fd, &read_fds_);
    if (fd > max_fd_) {
        max_fd_ = fd;
    }
}

void SelectDispatcher::remove_fd(int fd) {
    FD_CLR(fd, &read_fds_);
}

void SelectDispatcher::wait_and_handle_events(int timeout_milliseconds, const std::function<void(int fd, bool is_readable)>& handler) {
    fd_set temp_fds = read_fds_;
    timeval timeout{};
    timeout.tv_sec = 0;
    timeout.tv_usec = timeout_milliseconds * 1000;

    //LOG_TRACE("select wait for event.");
    int activity = select(max_fd_ + 1, &temp_fds, nullptr, nullptr, &timeout);
    if (activity > 0) {
        for (int i = 0; i <= max_fd_; ++i) {
            if (FD_ISSET(i, &temp_fds)) {
                handler(i, true);  // 假设所有事件都为可读事件
            }
        }
    }
}
