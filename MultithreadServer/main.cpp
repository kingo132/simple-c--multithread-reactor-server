#include "server.h"
#include "configuration_manager.h"
#include "log_manager.h"
#include "daemon_manager.h"
#include "dll_functions.h"
#include <iostream>

extern volatile int stop_signal;
LogManager* g_log_manager;

// Function to print usage instructions
void print_usage() {
    std::cout << "Usage: ./server [config_file_path] [dll_file_path]\n";
    std::cout << "Options:\n";
    std::cout << "  -h                   Show this help message\n";
    std::cout << "  config_file_path      Path to the configuration file (default: ./config.ini)\n";
    std::cout << "  dll_file_path         Path to the DLL file (default: ./libhandler.so)\n";
}

int main(int argc, char** argv) {
    // Default file paths
    std::string config_file = "./config.ini";
    std::string dll_file = "./libhandler.so";

    // Parse command-line arguments
    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1 == "-h") {
            print_usage();
            return 0;
        } else {
            config_file = argv[1];  // First argument is the configuration file path
        }
    }

    if (argc > 2) {
        dll_file = argv[2];  // Second argument is the DLL file path
    }

    // Load the configuration file
    ConfigurationManager config_manager;
    if (!config_manager.load_configuration(config_file)) {
        std::cerr << "Failed to load configuration from " << config_file << std::endl;
        return 1;
    }

    // Daemonize the server if configured to run in the background
    if (config_manager.get_string("run_mode", "foreground") == "background") {
        if (start_daemon(argc, argv) < 0) {
            std::cerr << "Failed to start as daemon" << std::endl;
            return 1;
        }
    }

    // Initialize logger
    g_log_manager = new LogManager(
        config_manager.get_string("log_dir", "./log"),
        config_manager.get_integer("log_level", 7),
        config_manager.get_integer("log_maxfiles", 10),
        config_manager.get_integer("log_size", 104857600),
        LogDestination::File
    );

    // Load the DLL functions
    dll_func_t dll_functions;
    if (!load_dll_functions(&dll_functions, dll_file.c_str())) {
        LOG_ERR("Error loading DLL functions from %s", dll_file.c_str());
        return 1;
    }

    // Start the server
    Server server(config_manager.get_integer("ringqueue_length", 8192000), config_manager.get_integer("worker_num", 4), &dll_functions);
    if (server.start(config_manager.get_string("bind_file", "./conf/bind.conf")) != 0) {
        LOG_ERR("Server start failed!");
        return 1;
    }

    auto last_timer_call = std::chrono::steady_clock::now();
    int timer_interval_ms = 1000;
    while (!stop_signal) {
        std::this_thread::sleep_for(std::chrono::milliseconds(timer_interval_ms));
        auto now = std::chrono::steady_clock::now();
        int elapsed_time = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(now - last_timer_call).count());
        dll_functions.handle_timer(&elapsed_time);
        last_timer_call = now;
    }

    // Stop the server
    server.stop();
    stop_daemon();

    return 0;
}
