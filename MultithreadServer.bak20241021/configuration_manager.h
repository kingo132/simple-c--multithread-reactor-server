#ifndef CONFIGURATION_MANAGER_H
#define CONFIGURATION_MANAGER_H

#include <string>
#include <unordered_map>

class ConfigurationManager {
public:
    static ConfigurationManager& getInstance();
    
    // Loads configuration from a file
    bool load_configuration(const std::string& file_path);

    // Retrieves a string configuration value
    std::string get_string(const std::string& key, const std::string& default_value = "") const;

    // Retrieves an integer configuration value
    int get_integer(const std::string& key, int default_value) const;

private:
    std::unordered_map<std::string, std::string> settings_;
};

#endif // CONFIGURATION_MANAGER_H
