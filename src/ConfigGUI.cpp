#include "ConfigGUI.hpp"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

bool ConfigGUI::ShowDialog(AppSettings& settings) {
    std::cout << "[GUI] Launching configuration dialog..." << std::endl;

#ifdef _WIN32
    // For a real production app, we would use a DialogBox or a GUI library like Qt/ImGui.
    // As a console-based alpha, we can use simple MessageBoxes or just read from stdin
    // to simulate the configuration interaction for now.

    int result = MessageBox(NULL, "Do you want to run as Server? (No for Client)", "NetMux Setup", MB_YESNOCANCEL | MB_ICONQUESTION);
    if (result == IDCANCEL) return false;

    settings.isServer = (result == IDYES);

    // In a real implementation, we would show a custom dialog with Edit controls for IP/Port/Boundary.
    // For now, we use these hardcoded defaults if not already set,
    // or keep existing settings if they were loaded from file.

    if (settings.port == 0) settings.port = 5555;
    if (settings.remoteIp.empty()) settings.remoteIp = "127.0.0.1";

    std::cout << "[GUI] Configured as " << (settings.isServer ? "Server" : "Client") << std::endl;
#endif

    return true;
}
