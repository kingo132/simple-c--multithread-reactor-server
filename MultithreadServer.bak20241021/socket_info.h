#ifndef SOCKET_INFO_H
#define SOCKET_INFO_H

#include <ctime>
#include <cstdint>

// Structure to hold metadata information for a socket, including block types
struct SocketInfo {
    int sock_fd;                // Socket file descriptor
    int socket_type;            // Type of the socket
    time_t recv_timestamp;      // Timestamp for when data was received
    time_t send_timestamp;      // Timestamp for when data was sent

    uint32_t local_ip;          // Local IP address
    uint16_t local_port;        // Local port number
    uint32_t remote_ip;         // Remote IP address
    uint16_t remote_port;       // Remote port number
};

#endif // SOCKET_INFO_H
