#pragma once
#include "ConfigManager.hpp"

#include <map>
#include "SyncModule.hpp"

class ConfigGUI {
public:
    static bool ShowDialog(AppSettings& settings, SyncModule* sync = nullptr);
    static void UpdateCursorMonitor(const std::map<unsigned long long, PeerState>& peers);

    // Lifecycle management for integrated execution
    static void Initialize(AppSettings& settings, SyncModule* sync);
    static void Tick();
    static bool IsRunning();
    static void Shutdown();
};
