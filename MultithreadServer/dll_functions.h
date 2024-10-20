#ifndef DLL_FUNCTIONS_H
#define DLL_FUNCTIONS_H

#include "socket_info.h"

// Structure for holding DLL function pointers
typedef struct dll_func_struct {
    void* handle;
    int (*handle_init)(int argc, char** argv, int thread_type);
    int (*handle_input)(const char* receive_buffer, int receive_buffer_len, const SocketInfo*);
    int (*handle_process)(const char* queue_block_data, int data_len, char** send_data, int* send_data_len, const SocketInfo*);
    int (*handle_open)(char** send_buffer, int* send_buffer_len, const SocketInfo*);
    int (*handle_close)(const SocketInfo*);
    int (*handle_timer)(int* milliseconds);
    void (*handle_fini)(int thread_type);
} dll_func_t;

// Loads the shared library and assigns function pointers
bool load_dll_functions(dll_func_t* dll_functions, const char* dll_path);

// Unloads the shared library
void unload_dll_functions(dll_func_t* dll_functions);

#endif // DLL_FUNCTIONS_H
