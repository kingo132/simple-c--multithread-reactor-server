#include "client_manager.h"
#include "log_manager.h"
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

void ClientManager::add_client(int client_fd, const SocketInfo& socket_info, uint32_t flags) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    ClientInfo client;
    client.socket_info = socket_info;
    client.recv_len = 0;
    client.send_len = 0;
    client.pending_close = false;
    client.flag = flags;

    clients_[client_fd] = client;

    LOG_INFO("Client added: %d", client_fd);
}

void ClientManager::remove_client(int client_fd, EventDispatcher* dispatcher) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    if (clients_.find(client_fd) != clients_.end()) {
        clients_.erase(client_fd);

        dispatcher->remove_fd(client_fd);

        LOG_INFO("Client removed: %d", client_fd);
    } else {
        LOG_WARN("Attempted to remove non-existing client: %d", client_fd);
    }
}

ClientInfo* ClientManager::get_client(int client_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto it = clients_.find(client_fd);
    if (it != clients_.end()) {
        return &it->second;
    }

    LOG_WARN("Client not found: %d", client_fd);
    return nullptr;
}

bool ClientManager::send_to_client(int client_fd, const char* data, size_t length) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto it = clients_.find(client_fd);
    if (it != clients_.end()) {
        ClientInfo& client = it->second;

        // If there's still unsent data, cannot send new data yet
        if (client.send_len > 0) {
            return false;
        }

        // Copy data to the send buffer
        std::memcpy(client.send_buffer, data, length);
        client.send_len = length;

        // Attempt to send data
        ssize_t bytes_sent = send(client_fd, client.send_buffer, client.send_len, 0);
        if (bytes_sent > 0) {
            // Update the remaining data to send
            client.send_len -= bytes_sent;
            if (client.send_len > 0) {
                std::memmove(client.send_buffer, client.send_buffer + bytes_sent, client.send_len);
            }
            LOG_DEBUG("Sent %zd bytes to client: %d", bytes_sent, client_fd);
            return true;
        } else if (bytes_sent == 0) {
            // Connection closed
            LOG_INFO("Connection closed by client: %d", client_fd);
            return false;
        } else {
            // Sending failed
            LOG_ERR("Failed to send data to client: %d", client_fd);
            return false;
        }
    } else {
        LOG_WARN("Attempted to send data to non-existing client: %d", client_fd);
        return false;
    }
}

// Get all clients
std::unordered_map<int, ClientInfo>& ClientManager::get_all_clients() {
    return clients_;
}
