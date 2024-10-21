#ifndef DEFAULT_CONFIG_H
#define DEFAULT_CONFIG_H

#include <string>

// Default configuration values

// Log Configuration
constexpr char DEFAULT_LOG_DIR[] = "./log";          // Directory for log files
constexpr int DEFAULT_LOG_LEVEL = 7;                 // Default log level (e.g., DEBUG level)
constexpr int DEFAULT_LOG_MAXFILES = 10;             // Maximum number of log files to keep
constexpr int DEFAULT_LOG_SIZE = 104857600;          // Maximum size of each log file in bytes (100 MB)
constexpr int DEFAULT_LOG_DEST = 3;

// Server Configuration
constexpr int DEFAULT_RINGQUEUE_LENGTH = 8192000;    // Length of ring queue buffer
constexpr int DEFAULT_WORKER_NUM = 4;                // Number of worker threads
constexpr char DEFAULT_BIND_FILE[] = "./conf/bind.txt"; // Path to bind configuration file

// Network Configuration
constexpr int DEFAULT_RECV_BUFFER_SIZE = 8196;       // Default size for receive buffers
constexpr int DEFAULT_SEND_BUFFER_SIZE = 8196;       // Default size for send buffers
constexpr int DEFAULT_MAX_PACKET_SIZE = 8196;        // Maximum packet size to be handled

// Daemon Configuration
constexpr char DEFAULT_RUN_MODE[] = "foreground";    // Default run mode (foreground or background)

#endif // DEFAULT_CONFIG_H
