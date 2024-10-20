#include "udp_handler.h"
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

// UDP doesn't require accepting clients in the same way as TCP
void UdpHandler::accept_client(int server_fd, ClientManager& client_manager, EventDispatcher* dispatcher, dll_func_t* dll_functions) {
    LOG_WARN("UDP does not accept new clients in the same manner as TCP. Ignoring accept_client for fd: %d", server_fd);
}

// Handle receiving UDP data
ssize_t UdpHandler::receive_data(ClientInfo& client, dll_func_t* dll_functions) {
    sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[RECV_BUFFER_SIZE];
    ssize_t bytes_received = recvfrom(client.socket_info.sock_fd, buffer, sizeof(buffer), 0, (sockaddr*)&client_addr, &client_addr_len);

    if (bytes_received > 0) {
        int result = dll_functions->handle_input(buffer, (int) bytes_received, &client.socket_info);
        if (result == 0) {
            // Continue receiving
            return 0;
        } else if (result > 0) {
            // Handle complete packet and push to the queue
            LOG_INFO("Received complete UDP packet from client fd: %d", client.socket_info.sock_fd);
            return bytes_received;
        } else {
            // Close the connection or stop receiving
            LOG_WARN("Closing UDP client fd: %d", client.socket_info.sock_fd);
            return -1;
        }
    } else {
        LOG_ERR("Error receiving UDP data on fd: %d", client.socket_info.sock_fd);
        return -1;
    }
}

// Handle sending UDP data
ssize_t UdpHandler::send_data(ClientInfo& client, const char* buffer, size_t length) {
    sockaddr_in client_addr{};
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(client.socket_info.local_port);
    client_addr.sin_addr.s_addr = htonl(client.socket_info.local_ip);

    ssize_t bytes_sent = sendto(client.socket_info.sock_fd, buffer, length, 0, (sockaddr*)&client_addr, sizeof(client_addr));

    if (bytes_sent >= 0) {
        LOG_INFO("Sent %ld bytes to UDP client fd: %d", bytes_sent, client.socket_info.sock_fd);
        return bytes_sent;
    } else {
        LOG_ERR("Failed to send data to UDP client fd: %d", client.socket_info.sock_fd);
        return -1;
    }
}
