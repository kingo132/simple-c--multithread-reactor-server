#ifndef SERVER_H
#define SERVER_H

#include <atomic>
#include <thread>
#include <vector>
#include <unordered_map>
#include <netinet/in.h>
#include "ring_queue.h"
#include "client_manager.h"
#include "event_dispatcher.h"
#include "dll_functions.h"
#include "log_manager.h"
#include "protocol_handler.h"

enum class ThreadType {
    MAIN = 0,
    CONN,
    WORK
};

struct BindInfo {
    std::string ip;
    int port;
    std::string type; // "tcp" or "udp"
    int idle_timeout;
    int flags;
};

class Server {
public:
    Server(size_t queue_size, int num_workers, dll_func_t* dll_funcs);
    ~Server();

    // Start and stop the server
    int start(const std::string& bind_file);
    void stop();
    
    void close_client_connectio(SocketInfo* si);
    void save_argc_argv(int argc, char** argv) {
        saved_argc_ = argc;
        saved_argv_ = argv;
    }

private:
    RingQueue recv_queue_; // Receive queue (network thread -> worker threads)
    RingQueue send_queue_; // Send queue (worker threads -> network thread)
    int num_workers_; // Number of worker threads
    std::atomic<bool> stop_flag_; // Flag to stop server
    std::thread network_thread_; // Thread handling network events
    std::vector<std::thread> worker_threads_; // Worker threads
    std::vector<int> server_sockets_;  // Handles multiple socket types (TCP/UDP)
    std::unordered_map<int, BindInfo> socket_bind_map_; // Maps socket FD to BindInfo for protocol type
    std::vector<BindInfo> binds_; // Stores parsed bind information
    dll_func_t* dll_functions_; // DLL function pointers
    ClientManager client_manager_; // Manages client connections
    EventDispatcher* dispatcher_; // Event dispatcher (epoll/select)
    ssize_t recv_buffer_size_;
    ssize_t send_buffer_size_;
    int saved_argc_;
    char** saved_argv_;

    // Create and bind server sockets based on configuration
    int create_server_sockets(const std::string& bind_file);

    // Main thread for handling network events
    void network_thread_func();

    // Worker threads for processing messages
    void worker_thread_func(int worker_id);

    // Get protocol handler based on connection type (TCP/UDP)
    ProtocolHandler* get_protocol_handler(int flags);

    // Handle client data, including new connections and data transmission
    void handle_client_data(int fd, bool is_readable);
};

#endif // SERVER_H
