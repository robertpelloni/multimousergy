#pragma once
#include "ConfigManager.hpp"

#include <map>
#include "SyncModule.hpp"

class ConfigGUI {
public:
    static bool ShowDialog(AppSettings& settings, SyncModule* sync = nullptr);
};
