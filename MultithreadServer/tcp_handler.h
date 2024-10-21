#ifndef TCP_HANDLER_H
#define TCP_HANDLER_H

#include "protocol_handler.h"

class TcpHandler : public ProtocolHandler {
public:
    void accept_client(int server_fd, ClientManager& client_manager, EventDispatcher* dispatcher, dll_func_t* dll_functions, size_t recv_buffer_size, size_t send_buffer_size) override;
    ssize_t receive_data(ClientInfo& client, dll_func_t* dll_functions, RingQueue& recv_queue) override;
    ssize_t send_data(ClientInfo& client, const char* buffer, size_t length) override;
};

#endif // TCP_HANDLER_H
