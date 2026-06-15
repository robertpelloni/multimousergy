#include "ConfigManager.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

ConfigManager::ConfigManager(const std::string& filename) : m_filename(filename) {}

static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool ConfigManager::Load(AppSettings& settings) {
    std::ifstream file(m_filename);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        size_t sep = line.find('=');
        if (sep == std::string::npos) continue;

        std::string key = trim(line.substr(0, sep));
        std::string val = trim(line.substr(sep + 1));

        if (key == "server_mode") settings.isServer = (val == "1" || val == "true");
        else if (key == "remote_ip") settings.remoteIp = val;
        else if (key == "port") settings.port = std::stoi(val);
        else if (key == "boundary_x") settings.inputConfig.boundaryX = std::stoi(val);
        else if (key == "boundary_y") settings.inputConfig.boundaryY = std::stoi(val);
        else if (key == "is_left") settings.inputConfig.isLeft = (val == "1" || val == "true");
        else if (key == "driver_type") settings.driverType = (NetMuxDriverType)std::stoi(val);
        else if (key == "group_id") settings.groupId = (unsigned int)std::stoul(val);
        else if (key == "group_name") settings.groupName = val;
        else if (key == "session_name") settings.sessionName = val;
        else if (key == "security_key") settings.securityKey = val;
        else if (key == "cursor_theme_path") settings.cursorThemePath = val;
        else if (key == "sel_color_r") settings.selectionColorR = (unsigned char)std::stoi(val);
        else if (key == "sel_color_g") settings.selectionColorG = (unsigned char)std::stoi(val);
        else if (key == "sel_color_b") settings.selectionColorB = (unsigned char)std::stoi(val);
        else if (key == "recent_servers") {
            std::stringstream ss(val);
            std::string ip;
            settings.recentServers.clear();
            while (std::getline(ss, ip, ',')) settings.recentServers.push_back(ip);
        }
    }

    return true;
}

bool ConfigManager::Save(const AppSettings& settings) {
    std::ofstream file(m_filename);
    if (!file.is_open()) return false;

    file << "# NetMux Configuration\n";
    file << "server_mode=" << (settings.isServer ? "true" : "false") << "\n";
    file << "remote_ip=" << settings.remoteIp << "\n";
    file << "port=" << settings.port << "\n";
    file << "boundary_x=" << settings.inputConfig.boundaryX << "\n";
    file << "boundary_y=" << settings.inputConfig.boundaryY << "\n";
    file << "is_left=" << (settings.inputConfig.isLeft ? "true" : "false") << "\n";
    file << "driver_type=" << (int)settings.driverType << "\n";
    file << "group_id=" << settings.groupId << "\n";
    file << "group_name=" << settings.groupName << "\n";
    file << "session_name=" << settings.sessionName << "\n";
    file << "security_key=" << settings.securityKey << "\n";
    file << "cursor_theme_path=" << settings.cursorThemePath << "\n";
    file << "sel_color_r=" << (int)settings.selectionColorR << "\n";
    file << "sel_color_g=" << (int)settings.selectionColorG << "\n";
    file << "sel_color_b=" << (int)settings.selectionColorB << "\n";

    file << "recent_servers=";
    for (size_t i = 0; i < settings.recentServers.size(); ++i) {
        file << settings.recentServers[i] << (i == settings.recentServers.size() - 1 ? "" : ",");
    }
    file << "\n";

    return true;
}
