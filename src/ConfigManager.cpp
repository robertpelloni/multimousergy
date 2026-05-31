#include "ConfigManager.hpp"
#include <fstream>
#include <iostream>

ConfigManager::ConfigManager(const std::string& filename) : m_filename(filename) {}

bool ConfigManager::Load(AppSettings& settings) {
    std::ifstream file(m_filename);
    if (!file.is_open()) return false;

    file >> settings.isServer;
    file >> settings.remoteIp;
    file >> settings.port;
    file >> settings.inputConfig.boundaryX;
    file >> settings.inputConfig.boundaryY;
    file >> settings.inputConfig.isLeft;
    int dt; file >> dt; settings.driverType = (NetMuxDriverType)dt;
    file >> settings.groupId;
    file >> settings.groupName;
    file >> settings.sessionName;
    file >> settings.securityKey;

    return true;
}

bool ConfigManager::Save(const AppSettings& settings) {
    std::ofstream file(m_filename);
    if (!file.is_open()) return false;

    file << settings.isServer << "\n";
    file << settings.remoteIp << "\n";
    file << settings.port << "\n";
    file << settings.inputConfig.boundaryX << "\n";
    file << settings.inputConfig.boundaryY << "\n";
    file << settings.inputConfig.isLeft << "\n";
    file << (int)settings.driverType << "\n";
    file << settings.groupId << "\n";
    file << settings.groupName << "\n";
    file << settings.sessionName << "\n";
    file << settings.securityKey << "\n";

    return true;
}
