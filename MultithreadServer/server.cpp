#include "server.h"
#include <cstring>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#ifdef __linux__
#include "epoll_dispatcher.h"
#define USE_EPOLL
#else
#include "select_dispatcher.h"
#endif

// Parse the bind configuration file and set flags
std::vector<BindInfo> parse_bind_file(const std::string& bind_file_path) {
    std::vector<BindInfo> binds;
    std::ifstream bind_file(bind_file_path);

    if (!bind_file.is_open()) {
        LOG_CRIT("Failed to open bind file: %s", bind_file_path.c_str());
        return binds;
    }

    std::string line;
    while (std::getline(bind_file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        BindInfo bind_info;
        iss >> bind_info.ip >> bind_info.port >> bind_info.type >> bind_info.idle_timeout;

        // Set the appropriate protocol flags based on the type
        if (bind_info.type == "tcp") {
            bind_info.flags = CN_LISTEN_MASK;  // TCP listen flag
        } else if (bind_info.type == "udp") {
            bind_info.flags = CN_LISTEN_MASK | CN_UDP_MASK;  // UDP listen flag
        } else {
            LOG_ERR("Unsupported protocol in bind file: %s", bind_info.type.c_str());
            continue;
        }

        binds.push_back(bind_info);
    }

    return binds;
}

// Server constructor
Server::Server(size_t queue_size, int num_workers, dll_func_t* dll_funcs)
    : queue_(queue_size), num_workers_(num_workers), stop_flag_(false), dll_functions_(dll_funcs) {
#ifdef USE_EPOLL
    dispatcher_ = new EpollDispatcher();
#else
    dispatcher_ = new SelectDispatcher();
#endif
}

// Server destructor
Server::~Server() {
    stop();
    delete dispatcher_;
}

// Start the server
int Server::start(const std::string& bind_file) {
    if (create_server_sockets(bind_file) != 0) {
        return -1;
    }

    network_thread_ = std::thread(&Server::network_thread_func, this);

    for (int i = 0; i < num_workers_; ++i) {
        worker_threads_.emplace_back(&Server::worker_thread_func, this, i);
    }
    
    return 0;
}

// Stop the server
void Server::stop() {
    stop_flag_.store(true);
    for (int socket : server_sockets_) {
        close(socket);
    }

    if (network_thread_.joinable()) {
        network_thread_.join();
    }

    for (auto& worker : worker_threads_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

// Create server sockets based on bind file
int Server::create_server_sockets(const std::string& bind_file) {
    binds_ = parse_bind_file(bind_file);  // Store bind configurations

    for (const auto& bind_info : binds_) {
        int sock_type = (bind_info.flags & CN_UDP_MASK) ? SOCK_DGRAM : SOCK_STREAM;
        int socket_fd = socket(AF_INET, sock_type, 0);
        if (socket_fd == -1) {
            LOG_CRIT("Failed to create server socket for %s:%d", bind_info.ip.c_str(), bind_info.port);
            return -1;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(bind_info.ip.c_str());
        server_addr.sin_port = htons(bind_info.port);

        if (bind(socket_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            LOG_CRIT("Failed to bind server socket for %s:%d", bind_info.ip.c_str(), bind_info.port);
            return -1;
        }

        if ((bind_info.flags & CN_LISTEN_MASK) && !(bind_info.flags & CN_UDP_MASK) && listen(socket_fd, 10) < 0) {
            LOG_CRIT("Failed to listen on server socket for %s:%d", bind_info.ip.c_str(), bind_info.port);
            return -1;
        }

        server_sockets_.push_back(socket_fd);
        socket_bind_map_[socket_fd] = bind_info;  // Map the socket to its bind info
        dispatcher_->add_fd(socket_fd);  // Add socket to the dispatcher
    }

    LOG_INFO("Server started and listening on configured ports");
    return 0;
}

// Unified protocol handler function using flags
ProtocolHandler* Server::get_protocol_handler(int flags) {
    if (flags & CN_UDP_MASK) {
        return ProtocolHandler::get_udp_handler();
    } else if (flags & CN_LISTEN_MASK) {
        return ProtocolHandler::get_tcp_handler();
    } else {
        LOG_CRIT("Unsupported protocol: %d", flags);
        return nullptr;
    }
}

// Handle accepting new TCP clients
void Server::accept_client(int server_socket, int flags) {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_len);

    if (client_socket >= 0) {
        SocketInfo socket_info;
        socket_info.sock_fd = client_socket;
        socket_info.local_ip = ntohl(client_addr.sin_addr.s_addr);
        socket_info.local_port = ntohs(client_addr.sin_port);

        dll_functions_->handle_open(nullptr, nullptr, &socket_info);

        // Add client to ClientManager
        client_manager_.add_client(client_socket, socket_info, flags);
        dispatcher_->add_fd(client_socket);  // Add client to event dispatcher
        LOG_INFO("Accepted new TCP client on fd: %d", client_socket);
    } else {
        LOG_ERR("Failed to accept new client");
    }
}

void Server::network_thread_func() {
    while (!stop_flag_.load(std::memory_order_acquire)) {
        dispatcher_->wait_and_handle_events([this](int fd, bool is_readable) {
            handle_client_data(fd, is_readable);
        });

        char buffer[RECV_BUFFER_SIZE];
        QueueBlock block;
        size_t actual_length;

        if (queue_.wait_and_pop(buffer, sizeof(buffer), actual_length, block, std::chrono::milliseconds(100))) {
            ClientInfo* client = client_manager_.get_client(block.socket_info.sock_fd);
            if (!client) {
                LOG_ERR("Failed to get client fd: %d", block.socket_info.sock_fd);
                continue;
            }

            ProtocolHandler* protocol_handler = get_protocol_handler(client->flag);
            if (protocol_handler) {
                if (block.type == BlockType::Data) {
                    int send_result = (int)protocol_handler->send_data(*client, buffer, actual_length);
                    if (send_result < 0) {
                        LOG_ERR("Failed to send data to client fd: %d", block.socket_info.sock_fd);
                    }
                } else if (block.type == BlockType::Final) {
                    if (client->send_len == 0) {
                        dll_functions_->handle_close(&client->socket_info);
                        client_manager_.remove_client(block.socket_info.sock_fd, dispatcher_);
                        close(block.socket_info.sock_fd);
                        LOG_INFO("Connection closed for client fd: %d", block.socket_info.sock_fd);
                    } else {
                        client->pending_close = true;
                    }
                }
            }
        }

        // Check for any pending closures
        for (auto& client : client_manager_.get_all_clients()) {
            if (client.second.pending_close && client.second.send_len == 0) {
                dll_functions_->handle_close(&client.second.socket_info);
                client_manager_.remove_client(client.first, dispatcher_);
                close(client.first);
                LOG_INFO("Connection finalized for client fd: %d", client.first);
            }
        }
    }
}

void Server::worker_thread_func(int worker_id) {
    while (!stop_flag_.load(std::memory_order_acquire)) {
        char buffer[RECV_BUFFER_SIZE];
        QueueBlock block;
        size_t actual_length;

        if (queue_.wait_and_pop(buffer, sizeof(buffer), actual_length, block, std::chrono::milliseconds(100))) {
            char* send_data = nullptr;
            int send_data_len = 0;

            int result = dll_functions_->handle_process(buffer, (int)actual_length, &send_data, &send_data_len, &block.socket_info);

            if (result >= 0 && send_data != nullptr) {
                QueueBlock response_block;
                response_block.accept_fd = block.accept_fd;
                response_block.socket_info = block.socket_info;
                response_block.type = BlockType::Data;
                response_block.total_length = send_data_len + sizeof(QueueBlock);

                queue_.push(send_data, send_data_len, response_block);
                LOG_DEBUG("Processed data for client fd: %d", block.socket_info.sock_fd);
            }

            if (result < 0) {
                QueueBlock final_block;
                final_block.accept_fd = block.accept_fd;
                final_block.socket_info = block.socket_info;
                final_block.type = BlockType::Final;
                final_block.total_length = sizeof(QueueBlock);
                queue_.push(nullptr, 0, final_block);
                LOG_WARN("Error processing data, pushing final block for fd: %d", block.socket_info.sock_fd);
            }
        }
    }
}

// Handle client data, including accepting new connections for TCP
void Server::handle_client_data(int fd, bool is_readable) {
    // Check if it's a server socket (for new connections)
    auto bind_info_it = socket_bind_map_.find(fd);
    if (bind_info_it != socket_bind_map_.end()) {
        // Use appropriate protocol handler to manage the connection
        ProtocolHandler* protocol_handler = get_protocol_handler(bind_info_it->second.flags);
        if (protocol_handler) {
            protocol_handler->accept_client(fd, client_manager_, dispatcher_, dll_functions_);
        } else {
            LOG_CRIT("Unsupported protocol for socket fd: %d", fd);
        }
        return;
    }

    // Handle client data for existing connections
    ClientInfo* client = client_manager_.get_client(fd);
    if (!client) {
        LOG_ERR("Failed to find client fd: %d", fd);
        return;
    }

    ProtocolHandler* protocol_handler = get_protocol_handler(client->flag);
    if (protocol_handler && is_readable) {
        int recv_result = (int) protocol_handler->receive_data(*client, dll_functions_);
        if (recv_result < 0) {
            LOG_ERR("Failed to receive data from client fd: %d", fd);
        }
    }
}
