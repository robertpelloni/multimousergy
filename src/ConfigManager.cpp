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
    file >> settings.groupId;
    file >> settings.groupName;
    file >> settings.sessionName;

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
    file << settings.groupId << "\n";
    file << settings.groupName << "\n";
    file << settings.sessionName << "\n";

    return true;
}
