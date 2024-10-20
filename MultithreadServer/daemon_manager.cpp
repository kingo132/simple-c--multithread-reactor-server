#include "daemon_manager.h"
#include "log_manager.h"
#include <csignal>
#include <unistd.h>
#include <sys/resource.h>
#include <cstring>
#include <iostream>

volatile int stop_signal = 0;
volatile int restart_signal = 0;

static void handle_stop_signal(int signo) {
    stop_signal = 1;
    restart_signal = 0;
}

static void handle_restart_signal(int signo) {
    restart_signal = 1;
    stop_signal = 1;
}

int start_daemon(int argc, char** argv) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_stop_signal;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    sa.sa_handler = handle_restart_signal;
    sigaction(SIGHUP, &sa, nullptr);

#ifdef __linux__
    if (daemon(1, 0) == -1) {
        std::cerr << "Failed to switch to daemon mode" << std::endl;
        return -1;
    }
#else
    std::cerr << "Warning: Daemonization is not supported on macOS. Running in foreground." << std::endl;
#endif

    return 0;
}

void stop_daemon() {
    std::cout << "Stopping server..." << std::endl;
}

