#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <string>
#include <mutex>
#include <fstream>
#include <cstdarg>

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
    LogManager(const std::string& log_dir, int log_level, int max_log_files, int max_log_size, LogDestination destination);
    ~LogManager();

    // Logs a message at the specified log level with printf-like formatting
    void log_message(LogLevel level, const char* format, ...);

private:
    std::string get_current_timestamp();
    void rotate_log_files();

    std::string log_directory_;
    std::ofstream log_file_;
    int log_level_;
    int max_log_files_;
    int max_log_size_;
    LogDestination log_destination_;
    std::mutex log_mutex_;
    int current_log_size_;
    int log_file_index_;
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

extern LogManager* g_log_manager;  // Assumed global log_manager

#endif // LOG_MANAGER_H
