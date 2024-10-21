#ifndef SERVER_API_H
#define SERVER_API_H

#include <string>

#ifdef _WIN32
    #define WEAK_SYMBOL __declspec(selectany)
    #define EXPORT_SYMBOL __declspec(dllexport)
#elif defined(__APPLE__)
    #define WEAK_SYMBOL __attribute__((weak_import))
    #define EXPORT_SYMBOL __attribute__((visibility("default")))
#else
    #define WEAK_SYMBOL __attribute__((weak))
    #define EXPORT_SYMBOL __attribute__((visibility("default")))
#endif

enum class LogLevel {
    Emergency = 0,
    Alert,
    Critical,
    Error,
    Warning,
    Notice,
    Info,
    Debug,
    Trace
};

enum class LogDestination {
    Terminal = 1,
    File = 2,
    Both = 3
};

class LogManager {
public:
    WEAK_SYMBOL void log_message(LogLevel level, const char* format, ...);
};

// Define macros to make logging simpler
#define LOG_EMERG(fmt, ...)   g_log_manager->log_message(LogLevel::Emergency, fmt, ##__VA_ARGS__)
#define LOG_ALERT(fmt, ...)   g_log_manager->log_message(LogLevel::Alert, fmt, ##__VA_ARGS__)
#define LOG_CRIT(fmt, ...)    g_log_manager->log_message(LogLevel::Critical, fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)     g_log_manager->log_message(LogLevel::Error, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)    g_log_manager->log_message(LogLevel::Warning, fmt, ##__VA_ARGS__)
#define LOG_NOTICE(fmt, ...)  g_log_manager->log_message(LogLevel::Notice, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)    g_log_manager->log_message(LogLevel::Info, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)   g_log_manager->log_message(LogLevel::Debug, fmt, ##__VA_ARGS__)
#define LOG_TRACE(fmt, ...)   g_log_manager->log_message(LogLevel::Trace, fmt, ##__VA_ARGS__)

WEAK_SYMBOL extern LogManager* g_log_manager;

class ConfigurationManager {
public:
    std::string get_string(const std::string& key, const std::string& default_value = "") const;
    int get_integer(const std::string& key, int default_value) const;
};

struct SocketInfo {
    int sock_fd;                // Socket file descriptor
    int socket_type;            // Type of the socket
    time_t recv_timestamp;      // Timestamp for when data was received
    time_t send_timestamp;      // Timestamp for when data was sent

    uint32_t local_ip;          // Local IP address
    uint16_t local_port;        // Local port number
    uint32_t remote_ip;         // Remote IP address
    uint16_t remote_port;       // Remote port number
};

#ifdef __cplusplus
extern "C" {
#endif

EXPORT_SYMBOL int handle_init(int argc, char** argv, int thread_type);
EXPORT_SYMBOL int handle_input(const char* receive_buffer, int receive_buffer_len, const SocketInfo*);
EXPORT_SYMBOL int handle_process(const char* queue_block_data, int data_len, char** send_data, int* send_data_len, const SocketInfo*);
EXPORT_SYMBOL int handle_open(char** send_buffer, int* send_buffer_len, const SocketInfo*);
EXPORT_SYMBOL int handle_close(const SocketInfo*);
EXPORT_SYMBOL int handle_timer(int* milliseconds);
EXPORT_SYMBOL void handle_fini(int thread_type);

#ifdef __cplusplus
}
#endif

#endif
