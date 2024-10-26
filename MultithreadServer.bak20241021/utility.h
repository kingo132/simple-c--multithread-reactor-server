#ifndef UTILITY_H
#define UTILITY_H

#include <string>

class Utility {
public:
    static std::string getCwd();
    static std::string trim(const std::string& str);
    static std::string get_current_timestamp_string();
};

#endif // UTILITY_H
