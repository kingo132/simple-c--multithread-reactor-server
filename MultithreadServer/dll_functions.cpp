#include "dll_functions.h"
#include "log_manager.h"
#include <dlfcn.h>
#include <iostream>
#include <cstring>

bool load_dll_functions(dll_func_t* dll_functions, const char* dll_path) {
    // Load the shared library
    dll_functions->handle = dlopen(dll_path, RTLD_NOW);
    if (!dll_functions->handle) {
        LOG_ERR("Failed to load DLL: %d", dlerror());
        return false;
    }

    // Clear any existing errors
    dlerror();

    // Assign function pointers from the DLL
    dll_functions->handle_init = (int (*)(int, char**, int))dlsym(dll_functions->handle, "handle_init");
    dll_functions->handle_input = (int (*)(const char*, int, const SocketInfo*))dlsym(dll_functions->handle, "handle_input");
    dll_functions->handle_process = (int (*)(const char*, int, char**, int*, const SocketInfo*))dlsym(dll_functions->handle, "handle_process");
    dll_functions->handle_open = (int (*)(char**, int*, const SocketInfo*))dlsym(dll_functions->handle, "handle_open");
    dll_functions->handle_close = (int (*)(const SocketInfo*))dlsym(dll_functions->handle, "handle_close");
    dll_functions->handle_timer = (int (*)(int*))dlsym(dll_functions->handle, "handle_timer");
    dll_functions->handle_fini = (void (*)(int))dlsym(dll_functions->handle, "handle_fini");

    // Check for any errors in loading symbols
    if (dlerror() != nullptr) {
        LOG_ERR("Failed to load function symbols from DLL");
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
