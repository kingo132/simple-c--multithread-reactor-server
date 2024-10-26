#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

#include <unordered_map>
#include <mutex>
#include "socket_info.h"
#include "event_dispatcher.h"

// Connection flags
constexpr uint32_t CN_VALID_MASK   = 0x01;
constexpr uint32_t CN_LISTEN_MASK  = 0x04;
constexpr uint32_t CN_PIPE_MASK    = 0x08;
constexpr uint32_t CN_UDP_MASK     = 0x10;
constexpr uint32_t CN_FINALIZE     = 0x20;

// Represents client connection information, including buffers and connection flags
struct ClientInfo {
    SocketInfo socket_info;      // Socket-related information (IP, port, etc.)
    char* recv_buffer;  // Buffer to hold incoming data
    char* send_buffer;  // Buffer to hold outgoing data
    size_t recv_buffer_size;
    size_t send_buffer_size;
    size_t recv_len;             // Length of valid data in receive buffer
    size_t send_len;             // Length of valid data in send buffer
    bool pending_close;          // Flag to mark if the connection should be closed
    uint32_t flag;               // Flags to describe connection type and state

    // Methods to check connection types
    bool is_udp() const { return (flag & CN_LISTEN_MASK) && (flag & CN_UDP_MASK); }
    bool is_tcp() const { return (flag & CN_LISTEN_MASK) && !(flag & CN_UDP_MASK); }
    bool is_finalize() const { return flag & CN_FINALIZE; }
    bool is_valid() const { return flag & CN_VALID_MASK; }
};

// Class to manage all client connections
class ClientManager {
public:
    // Add a client
    ClientInfo* add_client(int client_fd, const SocketInfo& socket_info, uint32_t flags, size_t recv_buffer_size, size_t send_buffer_size);

    // Remove a client
    void remove_client(int client_fd, EventDispatcher* dispatcher);

    // Get client information
    ClientInfo* get_client(int client_fd);

    // Send data to the client
    bool send_to_client(int client_fd, const char* data, size_t length);

    // Get all clients
    std::unordered_map<int, ClientInfo>& get_all_clients();

private:
    std::unordered_map<int, ClientInfo> clients_;  // Stores all client connections
    std::mutex clients_mutex_;  // Mutex lock to protect the client map
};

#endif // CLIENT_MANAGER_H
