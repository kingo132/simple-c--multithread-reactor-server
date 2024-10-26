#include "server.h"

#include <cstring>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "configuration_manager.h"
#include "default_config.h"
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
    : recv_queue_(queue_size), send_queue_(queue_size), num_workers_(num_workers), stop_flag_(false), dll_functions_(dll_funcs) {
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
    
    int max_pkt_size = ConfigurationManager::getInstance().get_integer("max_packet_size", DEFAULT_MAX_PACKET_SIZE);
    if (max_pkt_size > DEFAULT_MAX_PACKET_SIZE) {
        LOG_ERR("Max packet size %d exceed %d.", max_pkt_size, DEFAULT_MAX_PACKET_SIZE);
        return -1;
    }
    recv_buffer_size_ = ConfigurationManager::getInstance().get_integer("recv_buffer", DEFAULT_RECV_BUFFER_SIZE);
    send_buffer_size_ = ConfigurationManager::getInstance().get_integer("send_buffer", DEFAULT_SEND_BUFFER_SIZE);

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
    binds_ = parse_bind_file(bind_file);
    if (binds_.empty()) {
        return -1;
    }

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
        
        LOG_INFO("Listen on %s:%d (type: %s, idle: %d, flag: %d)",
                 bind_info.ip.c_str(), bind_info.port, bind_info.type.c_str(), bind_info.idle_timeout,
                 bind_info.flags);
    }

    LOG_INFO("Server started!");
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

void Server::close_client_connection(SocketInfo* si) {
    if (dll_functions_->handle_client_close) {
        dll_functions_->handle_client_close(si);
    }
    client_manager_.remove_client(si->sock_fd, dispatcher_);
    dispatcher_->remove_fd(si->sock_fd);
    close(si->sock_fd);
}

void Server::network_thread_func() {
    if (dll_functions_->handle_init && dll_functions_->handle_init(saved_argc_, saved_argv_, (int) ThreadType::CONN) != 0) {
        LOG_ERR("Network thread handle_init failed.");
        return;
    }
    
    while (!stop_flag_.load(std::memory_order_acquire)) {
        // 1. Wait for the network event, wait maximum for 100 milliseconds
        dispatcher_->wait_and_handle_events(100, [this](int fd, bool is_readable) {
            handle_client_data(fd, is_readable);
        });

        char buffer[DEFAULT_MAX_PACKET_SIZE];
        QueueBlock block;
        size_t actual_length;

        // 2. Pop data from the send queue to send to clients
        if (send_queue_.wait_and_pop(buffer, sizeof(buffer), actual_length, block, std::chrono::milliseconds(100))) {
            ClientInfo* client = client_manager_.get_client(block.socket_info.sock_fd);
            if (!client) {
                LOG_TRACE("Failed to get client fd: %d", block.socket_info.sock_fd);
                continue;
            }

            ProtocolHandler* protocol_handler = get_protocol_handler(client->flag);
            if (protocol_handler) {
                if (block.type == BlockType::Data) {
                    int send_result = (int)protocol_handler->send_data(*client, buffer, actual_length);
                    if (send_result < 0) {
                        LOG_ERR("Failed to send data to client fd: %d, close conn.", block.socket_info.sock_fd);
                        close_client_connection(&client->socket_info);
                    }
                } else if (block.type == BlockType::Final) {
                    if (client->send_len == 0) {
                        LOG_INFO("Connection closed for client fd: %d", block.socket_info.sock_fd);
                        close_client_connection(&client->socket_info);
                    } else {
                        client->pending_close = true;
                    }
                }
            }
        }

        // 3. Check for any pending closures client connections
        for (auto& client_pair : client_manager_.get_all_clients()) {
            ClientInfo& client = client_pair.second;
            
            // Send remaining data
            ProtocolHandler* protocol_handler = get_protocol_handler(client.flag);
            if (protocol_handler && client.send_len > 0) {
                int send_result = (int)protocol_handler->send_data(client, nullptr, 0);
                if (send_result < 0) {
                    LOG_ERR("Failed to send remaining data to client fd: %d", client.socket_info.sock_fd);
                    // Discard the remaining data
                    client.send_len = 0;
                    client.pending_close = true;
                }
            }

            // Close connections
            if (client.pending_close && client.send_len == 0) {
                LOG_INFO("Connection finalized for client fd: %d, close it.", client.socket_info.sock_fd);
                close_client_connection(&client.socket_info);
            }
        }
    }
    
    if (dll_functions_->handle_fini) {
        dll_functions_->handle_fini((int) ThreadType::CONN);
    }
}

void Server::worker_thread_func(int worker_id) {
    if (dll_functions_->handle_init && dll_functions_->handle_init(saved_argc_, saved_argv_, (int) ThreadType::WORK) != 0) {
        LOG_ERR("Work thread handle_init failed.");
        return;
    }
    
    while (!stop_flag_.load(std::memory_order_acquire)) {
        char buffer[DEFAULT_MAX_PACKET_SIZE];
        QueueBlock block;
        size_t actual_length;

        // Pop data from the receive queue to process
        if (recv_queue_.wait_and_pop(buffer, sizeof(buffer), actual_length, block, std::chrono::milliseconds(100))) {
            char send_buffer[DEFAULT_MAX_PACKET_SIZE];
            char* send_data = send_buffer;
            int send_data_len = 0;
            int result = dll_functions_->handle_message_from_client(buffer, (int)actual_length, &send_data, &send_data_len, &block.socket_info);

            if (result >= 0 && send_data != nullptr) {
                QueueBlock response_block;
                response_block.accept_fd = block.accept_fd;
                response_block.socket_info = block.socket_info;
                response_block.type = BlockType::Data;
                response_block.total_length = send_data_len + sizeof(QueueBlock);

                // Push processed data to the send queue
                send_queue_.push(send_data, send_data_len, response_block);
                LOG_TRACE("Processed data for client fd: %d", block.socket_info.sock_fd);
            }

            if (result < 0) {
                QueueBlock final_block;
                final_block.accept_fd = block.accept_fd;
                final_block.socket_info = block.socket_info;
                final_block.type = BlockType::Final;
                final_block.total_length = sizeof(QueueBlock);
                send_queue_.push(nullptr, 0, final_block);
                LOG_WARN("Error processing data, pushing final block for fd: %d", block.socket_info.sock_fd);
            }
        }
    }
    
    if (dll_functions_->handle_fini) {
        dll_functions_->handle_fini((int) ThreadType::WORK);
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
            protocol_handler->accept_client(fd, client_manager_, dispatcher_, dll_functions_, recv_buffer_size_, send_buffer_size_);
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
        int recv_result = (int) protocol_handler->receive_data(*client, dll_functions_, recv_queue_);
        if (recv_result < 0) {
            LOG_ERR("Failed to receive data from client fd: %d, close connection.", fd);
            close_client_connection(&client->socket_info);
        }
    }
}
