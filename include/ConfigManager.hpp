#pragma once
#include <string>
#include "CommonTypes.hpp"

class ConfigManager {
public:
    ConfigManager(const std::string& filename);

    bool Load(AppSettings& settings);
    bool Save(const AppSettings& settings);

private:
    std::string m_filename;
};
