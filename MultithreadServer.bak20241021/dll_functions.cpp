#include "dll_functions.h"
#include "log_manager.h"
#include <dlfcn.h>
#include <iostream>
#include <cstring>

#define LOAD_FUNCTION(handle, func, symbol) \
    func = (decltype(func))dlsym(handle, symbol)

bool load_dll_functions(dll_func_t* dll_functions, const char* dll_path) {
    // Load the shared library
    dll_functions->handle = dlopen(dll_path, RTLD_NOW);
    if (!dll_functions->handle) {
        LOG_ERR("Failed to load DLL: %d", dlerror());
        return false;
    }

    // Load each function
    LOAD_FUNCTION(dll_functions->handle, dll_functions->handle_init, "handle_init");
    LOAD_FUNCTION(dll_functions->handle, dll_functions->handle_input_from_client, "handle_input_from_client");
    LOAD_FUNCTION(dll_functions->handle, dll_functions->handle_message_from_client, "handle_message_from_client");
    LOAD_FUNCTION(dll_functions->handle, dll_functions->handle_client_open, "handle_client_open");
    LOAD_FUNCTION(dll_functions->handle, dll_functions->handle_client_close, "handle_client_close");
    LOAD_FUNCTION(dll_functions->handle, dll_functions->handle_timer, "handle_timer");
    LOAD_FUNCTION(dll_functions->handle, dll_functions->handle_fini, "handle_fini");
    
    // Check for mandatory interfaces
    if (! dll_functions->handle_input_from_client) {
        LOG_ERR("handle_input_from_client not implemented!");
        unload_dll_functions(dll_functions);
        return false;
    }
    if (! dll_functions->handle_message_from_client) {
        LOG_ERR("handle_message_from_client not implemented!");
        unload_dll_functions(dll_functions);
        return false;
    }

    return true;
}

// Function to unload the shared library
void unload_dll_functions(dll_func_t* dll_functions) {
    if (dll_functions->handle) {
        dlclose(dll_functions->handle);
        dll_functions->handle = nullptr;
    }
}
