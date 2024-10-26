#include "tcp_handler.h"

#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

#include "default_config.h"

// Handle new TCP client connection
void TcpHandler::accept_client(int server_fd, ClientManager& client_manager, EventDispatcher* dispatcher, dll_func_t* dll_functions, size_t recv_buffer_size, size_t send_buffer_size) {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);

    if (client_fd >= 0) {
        SocketInfo socket_info;
        socket_info.sock_fd = client_fd;
        socket_info.local_ip = ntohl(client_addr.sin_addr.s_addr);
        socket_info.local_port = ntohs(client_addr.sin_port);

        // Add client to ClientManager
        ClientInfo* ci = client_manager.add_client(client_fd, socket_info, CN_VALID_MASK | CN_LISTEN_MASK, recv_buffer_size, send_buffer_size);

        int send_len = (int) ci->send_len;
        if (dll_functions->handle_client_open && dll_functions->handle_client_open(&ci->send_buffer, &(send_len), &ci->socket_info) < 0) {
            LOG_TRACE("handle_client_open error, remove client.");
            client_manager.remove_client(client_fd, dispatcher);
            close(client_fd);
            return;
        }
        
        // Add client fd to the event dispatcher
        dispatcher->add_fd(client_fd);

        LOG_INFO("Accepted new TCP client: %d", client_fd);
    } else {
        LOG_ERR("Failed to accept new TCP client on server_fd: %d", server_fd);
    }
}

// Handle receiving TCP data
ssize_t TcpHandler::receive_data(ClientInfo& client, dll_func_t* dll_functions, RingQueue& recv_queue) {
    char buffer[DEFAULT_MAX_PACKET_SIZE];
    
    ssize_t bytes_received = recv(client.socket_info.sock_fd, buffer, DEFAULT_MAX_PACKET_SIZE, 0);

    if (bytes_received > 0) {
        LOG_TRACE("recv return len %d.", bytes_received);
        if (client.recv_len + bytes_received > client.recv_buffer_size) {
            LOG_ERR("Receive buffer overflow for client fd: %d", client.socket_info.sock_fd);
            return -1;
        }

        std::memcpy(client.recv_buffer + client.recv_len, buffer, bytes_received);
        client.recv_len += bytes_received;

        // Process the accumulated data
        int result;
        while ((result = dll_functions->handle_input_from_client(client.recv_buffer, (int)client.recv_len, &client.socket_info)) > 0) {
            // Handle complete packet
            LOG_TRACE("Received complete packet size %d from TCP client fd: %d", result, client.socket_info.sock_fd);

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
            if (client.recv_len == 0) {
                result = 0;
                break;
            }
        }

        // If `handle_input_from_client` returns 0, it means we're still waiting for more data.
        if (result == 0) {
            return 0;
        } else {
            // `handle_input_from_client` returned a negative value, indicating an error.
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
    ssize_t bytes_sent;

    // Step 1: If there is leftover data in send_buffer, append the new data to it
    if (client.send_len > 0) {
        if (client.send_len + length <= client.send_buffer_size) {
            std::memcpy(client.send_buffer + client.send_len, buffer, length);
            client.send_len += length;
        } else {
            LOG_ERR("Send buffer overflow for client fd: %d", client.socket_info.sock_fd);
            return -1; // Buffer overflow
        }

        bytes_sent = send(client.socket_info.sock_fd, client.send_buffer, client.send_len, 0);
    } else {
        // No leftover data, send new data directly
        bytes_sent = send(client.socket_info.sock_fd, buffer, length, 0);
    }

    // Step 2: Handle the result of the send operation
    if (bytes_sent >= 0) {
        LOG_TRACE("Sent %ld bytes to TCP client fd: %d", bytes_sent, client.socket_info.sock_fd);

        // If we sent part of the send_buffer, shift remaining data
        if (client.send_len > 0) {
            if ((size_t)bytes_sent < client.send_len) {
                size_t remaining_length = client.send_len - bytes_sent;
                std::memmove(client.send_buffer, client.send_buffer + bytes_sent, remaining_length);
                client.send_len = remaining_length;
            } else {
                client.send_len = 0;
            }
        } else if ((size_t)bytes_sent < length) {
            // Not all data was sent, store the unsent part in send_buffer
            size_t remaining_length = length - bytes_sent;
            if (remaining_length <= client.send_buffer_size) {
                std::memcpy(client.send_buffer, buffer + bytes_sent, remaining_length);
                client.send_len = remaining_length;
            } else {
                LOG_ERR("Send buffer overflow for client fd: %d", client.socket_info.sock_fd);
                return -1; // Buffer overflow
            }
        }
    } else {
        LOG_ERR("Failed to send data to TCP client fd: %d", client.socket_info.sock_fd);
        return -1; // Error occurred while sending data
    }

    return bytes_sent;
}

