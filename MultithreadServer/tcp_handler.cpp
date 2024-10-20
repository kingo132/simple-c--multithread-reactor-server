#include "tcp_handler.h"
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

// Handle new TCP client connection
void TcpHandler::accept_client(int server_fd, ClientManager& client_manager, EventDispatcher* dispatcher, dll_func_t* dll_functions) {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);

    if (client_fd >= 0) {
        SocketInfo socket_info;
        socket_info.sock_fd = client_fd;
        socket_info.local_ip = ntohl(client_addr.sin_addr.s_addr);
        socket_info.local_port = ntohs(client_addr.sin_port);

        // Add client to ClientManager
        client_manager.add_client(client_fd, socket_info, CN_VALID_MASK | CN_LISTEN_MASK);

        // Add client fd to the event dispatcher
        dispatcher->add_fd(client_fd);
        LOG_INFO("Accepted new TCP client: %d", client_fd);
    } else {
        LOG_ERR("Failed to accept new TCP client on server_fd: %d", server_fd);
    }
}

// Handle receiving TCP data
ssize_t TcpHandler::receive_data(ClientInfo& client, dll_func_t* dll_functions) {
    char buffer[RECV_BUFFER_SIZE];
    ssize_t bytes_received = recv(client.socket_info.sock_fd, buffer, sizeof(buffer), 0);

    if (bytes_received > 0) {
        int result = dll_functions->handle_input(buffer, (int) bytes_received, &client.socket_info);
        if (result == 0) {
            // Continue receiving
            return 0;
        } else if (result > 0) {
            // Handle complete packet and push to the queue
            LOG_INFO("Received complete packet from TCP client fd: %d", client.socket_info.sock_fd);
            return bytes_received;
        } else {
            // Close the connection
            LOG_WARN("Closing TCP connection on fd: %d", client.socket_info.sock_fd);
            return -1;
        }
    } else if (bytes_received == 0) {
        LOG_INFO("TCP client closed connection: %d", client.socket_info.sock_fd);
        return -1;
    } else {
        LOG_ERR("Error receiving data from TCP client fd: %d", client.socket_info.sock_fd);
        return -1;
    }
}

// Handle sending TCP data
ssize_t TcpHandler::send_data(ClientInfo& client, const char* buffer, size_t length) {
    ssize_t bytes_sent = send(client.socket_info.sock_fd, buffer, length, 0);

    if (bytes_sent >= 0) {
        LOG_INFO("Sent %ld bytes to TCP client fd: %d", bytes_sent, client.socket_info.sock_fd);
        return bytes_sent;
    } else {
        LOG_ERR("Failed to send data to TCP client fd: %d", client.socket_info.sock_fd);
        return -1;
    }
}
