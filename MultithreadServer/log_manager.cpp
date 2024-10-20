#include "log_manager.h"
#include <iostream>
#include <ctime>
#include <cstdarg>
#include <iomanip>
#include <sstream>

LogManager::LogManager(const std::string& log_dir, int log_level, int max_log_files, int max_log_size, LogDestination destination)
    : log_directory_(log_dir), log_level_(log_level), max_log_files_(max_log_files), max_log_size_(max_log_size),
      log_destination_(destination), current_log_size_(0), log_file_index_(0) {
    rotate_log_files();
}

LogManager::~LogManager() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void LogManager::log_message(LogLevel level, const char* format, ...) {
    if (static_cast<int>(level) > log_level_) {
        return;
    }

    std::lock_guard<std::mutex> lock(log_mutex_);

    // Create formatted log message using variable arguments
    char log_message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(log_message, sizeof(log_message), format, args);
    va_end(args);

    std::string timestamp = get_current_timestamp();
    std::string full_message = "[" + timestamp + "] " + log_message;

    if (log_destination_ == LogDestination::Terminal || log_destination_ == LogDestination::Both) {
        std::cout << full_message << std::endl;
    }

    if (log_destination_ == LogDestination::File || log_destination_ == LogDestination::Both) {
        log_file_ << full_message << std::endl;
        current_log_size_ += full_message.size();
        if (current_log_size_ > max_log_size_) {
            rotate_log_files();
        }
    }
}

std::string LogManager::get_current_timestamp() {
    time_t now = time(nullptr);
    struct tm* time_info = localtime(&now);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", time_info);
    return std::string(buffer);
}

void LogManager::rotate_log_files() {
    if (log_file_.is_open()) {
        log_file_.close();
    }

    std::time_t t = std::time(nullptr);
    std::tm local_tm = *std::localtime(&t);

    // log_YYYYMMDD_HHMM_index.txt
    std::ostringstream oss;
    oss << log_directory_ << "/log_"
        << std::put_time(&local_tm, "%Y%m%d_%H%M") << "_"
        << log_file_index_ << ".txt";

    log_file_index_ = (log_file_index_ % max_log_files_) + 1;
    std::string log_filename = oss.str();

    log_file_.open(log_filename, std::ios::out | std::ios::app);
    current_log_size_ = 0;
}
