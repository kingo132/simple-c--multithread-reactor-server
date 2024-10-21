#include "server_api.h"

int handle_init(int argc, char** argv, int thread_type) {
    return 0;
}

int handle_input(const char* receive_buffer, int receive_buffer_len, const SocketInfo* si) {
    if (receive_buffer_len > sizeof(uint32_t)) {
        uint32_t len = *((uint32_t*)receive_buffer);
        if (receive_buffer_len >= len) {
            LOG_TRACE("handle_input, len: %d => %d, %d:%d", receive_buffer_len, len, si->remote_ip, si->remote_port);
            return len;
        }
    }
    LOG_TRACE("handle_input, len: %d => 0, %d:%d", receive_buffer_len, si->remote_ip, si->remote_port);
    return 0;
}

int handle_process(const char* data, int data_len, char** send_data, int* send_data_len, const SocketInfo* si) {
    LOG_TRACE("handle_process, len: %d, %d:%d", data_len, si->remote_ip, si->remote_port);
    std::memcpy(*send_data, data, data_len);
    *send_data_len = data_len;
    return 0;
}

int handle_open(char** send_buffer, int* send_buffer_len, const SocketInfo* si) {
    return 0;
}

int handle_close(const SocketInfo* si) {
    return 0;
}

int handle_timer(int* milliseconds) {
    return 0;
}

void handle_fini(int thread_type) {
    
}
