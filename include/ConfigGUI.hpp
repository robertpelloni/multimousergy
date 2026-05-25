#pragma once
#include "ConfigManager.hpp"

#include <map>
#include "SyncModule.hpp"

class ConfigGUI {
public:
    static bool ShowDialog(AppSettings& settings, SyncModule* sync = nullptr);
    static void UpdateCursorMonitor(const std::map<unsigned long long, PeerState>& peers);
};
