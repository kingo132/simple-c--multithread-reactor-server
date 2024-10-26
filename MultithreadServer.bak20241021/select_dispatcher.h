#ifndef SELECT_DISPATCHER_H
#define SELECT_DISPATCHER_H

#include "event_dispatcher.h"
#include <sys/select.h>
#include <unistd.h>
#include <functional>

class SelectDispatcher : public EventDispatcher {
public:
    SelectDispatcher();
    ~SelectDispatcher() override = default;

    void add_fd(int fd) override;
    void remove_fd(int fd) override;
    void wait_and_handle_events(int timeout_milliseconds, const std::function<void(int fd, bool is_readable)>& handler) override;

private:
    fd_set read_fds_;
    int max_fd_;
};

#endif // SELECT_DISPATCHER_H
