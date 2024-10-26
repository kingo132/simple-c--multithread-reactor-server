#ifndef DLL_FUNCTIONS_H
#define DLL_FUNCTIONS_H

#include "socket_info.h"

// Structure for holding DLL function pointers
typedef struct dll_func_struct {
    void* handle;
    int (*handle_init)(int argc, char** argv, int thread_type);
    int (*handle_input_from_client)(const char* available_data, int available_data_len, const SocketInfo* si);
    int (*handle_input_from_server)(const char* available_data, int available_data_len, int fd);
    int (*handle_message_from_client)(const char* recvc_data, int recvc_data_len, char** send_data, int* send_data_len, const SocketInfo* si);
    int (*handle_message_from_server)(const char* recvc_data, int recvc_data_len, char** send_data, int* send_data_len, const SocketInfo* si);
    int (*handle_client_open)(char** send_buffer, int* send_buffer_len, const SocketInfo* si);
    int (*handle_client_close)(const SocketInfo* si);
    int (*handle_server_close)(int fd);
    int (*handle_timer)(int* milliseconds);
    void (*handle_fini)(int thread_type);
} dll_func_t;

// Loads the shared library and assigns function pointers
bool load_dll_functions(dll_func_t* dll_functions, const char* dll_path);

// Unloads the shared library
void unload_dll_functions(dll_func_t* dll_functions);

#endif // DLL_FUNCTIONS_H
