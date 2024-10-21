#include "utility.h"

#ifdef _WIN32
#include <direct.h>  // For Windows
#define GetCurrentDir _getcwd
#else
#include <unistd.h>  // For Unix-like systems
#define GetCurrentDir getcwd
#endif

#include <iostream>

#define MAX_PATH_LENGTH 1024

std::string Utility::getCwd() {
    char currentPath[MAX_PATH_LENGTH];
    if (GetCurrentDir(currentPath, sizeof(currentPath)) == nullptr) {
        return "";
    }
    return std::string(currentPath);
}

std::string Utility::trim(const std::string& str) {
    auto begin = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();

    return (begin < end) ? std::string(begin, end) : std::string();
}

std::string Utility::get_current_timestamp_string() {
    time_t now = time(nullptr);
    struct tm* time_info = localtime(&now);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", time_info);
    return std::string(buffer);
}
