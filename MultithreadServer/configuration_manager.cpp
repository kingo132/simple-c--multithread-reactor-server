#include "configuration_manager.h"
#include "log_manager.h"
#include "utility.h"
#include <fstream>
#include <sstream>
#include <iostream>

ConfigurationManager& ConfigurationManager::getInstance() {
    static ConfigurationManager cm;
    return cm;
}

bool ConfigurationManager::load_configuration(const std::string& file_path) {
    std::ifstream config_file(file_path);
    if (!config_file.is_open()) {
        std::cerr << "[" + Utility::get_current_timestamp_string() + "] Failed to open configuration file: " << file_path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(config_file, line)) {
        std::istringstream line_stream(line);
        std::string key, value;

        if (std::getline(line_stream, key, '=') && std::getline(line_stream, value)) {
            key = Utility::trim(key);
            value = Utility::trim(value);
            settings_[key] = value;
            
            std::cout << "[" + Utility::get_current_timestamp_string() + "] Load Conf " << key << ": " << value << std::endl;
        }
    }
    return true;
}

std::string ConfigurationManager::get_string(const std::string& key, const std::string& default_value) const {
    auto it = settings_.find(key);
    if (it != settings_.end()) {
        return it->second;
    }
    return default_value;
}

int ConfigurationManager::get_integer(const std::string& key, int default_value) const {
    auto it = settings_.find(key);
    if (it != settings_.end()) {
        return std::stoi(it->second);
    }
    return default_value;
}
