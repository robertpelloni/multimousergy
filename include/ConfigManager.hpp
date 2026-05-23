#pragma once
#include <string>
#include "InputEngine.hpp"

struct AppSettings {
    bool isServer;
    std::string remoteIp;
    int port;
    Config inputConfig;
    float cursorScale = 1.0f;
    bool useD3D11 = false;
};

class ConfigManager {
public:
    ConfigManager(const std::string& filename);

    bool Load(AppSettings& settings);
    bool Save(const AppSettings& settings);

private:
    std::string m_filename;
};
