#ifndef PROTOCOL_HANDLER_H
#define PROTOCOL_HANDLER_H

#include "client_manager.h"
#include "event_dispatcher.h"
#include "dll_functions.h"
#include "log_manager.h"

class ProtocolHandler {
public:
    virtual ~ProtocolHandler() = default;

    // Method to accept new clients
    virtual void accept_client(int server_fd, ClientManager& client_manager, EventDispatcher* dispatcher, dll_func_t* dll_functions) = 0;

    // Method to receive data
    virtual ssize_t receive_data(ClientInfo& client, dll_func_t* dll_functions) = 0;

    // Method to send data
    virtual ssize_t send_data(ClientInfo& client, const char* buffer, size_t length) = 0;

    // Static factory methods to get protocol handlers
    static ProtocolHandler* get_tcp_handler();
    static ProtocolHandler* get_udp_handler();
};

#endif // PROTOCOL_HANDLER_H
