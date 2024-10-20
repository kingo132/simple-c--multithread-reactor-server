#ifndef EVENT_DISPATCHER_H
#define EVENT_DISPATCHER_H

#include <functional>

class EventDispatcher {
public:
    virtual ~EventDispatcher() = default;

    // Add file descriptor for monitoring
    virtual void add_fd(int fd) = 0;

    // Remove file descriptor
    virtual void remove_fd(int fd) = 0;

    // Wait for and handle events, is_readable indicates if it is a read event
    virtual void wait_and_handle_events(const std::function<void(int fd, bool is_readable)>& handler) = 0;
};

#endif // EVENT_DISPATCHER_H
