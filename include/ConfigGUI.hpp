#pragma once
#include "ConfigManager.hpp"

#include <map>
#include "SyncModule.hpp"
#include "NetworkManager.hpp"

class ConfigGUI {
public:
    static bool ShowDialog(AppSettings& settings, SyncModule* sync = nullptr);
    static void UpdateCursorMonitor(const std::map<unsigned long long, PeerState>& peers);

    // Lifecycle management for integrated execution
    static void Initialize(AppSettings& settings, SyncModule* sync, NetworkManager* network = nullptr);
    static void SetSyncModule(SyncModule* sync);
    static void Tick();
    static bool IsRunning();
    static bool RestartRequested();
    static void Shutdown();

    static void LogSecurityEvent(const std::string& event);
};
