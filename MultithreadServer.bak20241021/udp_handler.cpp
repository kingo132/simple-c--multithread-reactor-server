#include "udp_handler.h"

#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

#include "default_config.h"

// UDP doesn't require accepting clients in the same way as TCP
void UdpHandler::accept_client(int server_fd, ClientManager& client_manager, EventDispatcher* dispatcher, dll_func_t* dll_functions, size_t recv_buffer_size, size_t send_buffer_size) {
    LOG_WARN("UDP does not accept new clients in the same manner as TCP. Ignoring accept_client for fd: %d", server_fd);
}

// Handle receiving UDP data
ssize_t UdpHandler::receive_data(ClientInfo& client, dll_func_t* dll_functions, RingQueue& recv_queue) {
    sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[DEFAULT_MAX_PACKET_SIZE];
    ssize_t bytes_received = recvfrom(client.socket_info.sock_fd, buffer, sizeof(buffer), 0, (sockaddr*)&client_addr, &client_addr_len);

    if (bytes_received > 0) {
        if (client.recv_len + bytes_received > client.recv_buffer_size) {
            LOG_ERR("Receive buffer overflow for client fd: %d", client.socket_info.sock_fd);
            return -1;
        }

        // Copy received data to client's receive buffer
        std::memcpy(client.recv_buffer + client.recv_len, buffer, bytes_received);
        client.recv_len += bytes_received;

        // Process the accumulated data
        int result;
        while ((result = dll_functions->handle_input_from_client(client.recv_buffer, (int)client.recv_len, &client.socket_info)) > 0) {
            // Handle complete packet
            LOG_INFO("Received complete UDP packet from client fd: %d", client.socket_info.sock_fd);

            // Push the complete packet to the queue
            QueueBlock recv_block;
            recv_block.accept_fd = client.socket_info.sock_fd;
            recv_block.socket_info = client.socket_info;
            recv_block.type = BlockType::Data;
            recv_block.total_length = result + sizeof(QueueBlock);

            recv_queue.push(client.recv_buffer, result, recv_block);

            // Shift any remaining data in recv_buffer
            size_t remaining_length = client.recv_len - result;
            if (remaining_length > 0) {
                std::memmove(client.recv_buffer, client.recv_buffer + result, remaining_length);
            }
            client.recv_len = remaining_length;
        }

        // If `handle_input_from_client` returns 0, it means we're still waiting for more data.
        if (result == 0) {
            return 0;
        } else {
            // `handle_input_from_client` returned a negative value, indicating an error.
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
