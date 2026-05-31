#include <iostream>
#include <algorithm>
#include "NetMuxFramework.hpp"
#include "ConfigGUI.hpp"

#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    #ifdef _WIN32
    SetProcessDPIAware();
    #endif
    std::cout << "NetMux starting..." << std::endl;

    ConfigManager configManager("netmux.cfg");
    AppSettings settings = { false, "127.0.0.1", 5555, {0, 0, false} };

    unsigned char colorR = 255, colorG = 0, colorB = 0;
    bool benchMode = false;
    bool autoConnect = false;
    bool firstRun = !configManager.Load(settings);

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--server") settings.isServer = true;
        else if (arg == "--client" && i + 1 < argc) { settings.remoteIp = argv[++i]; settings.isServer = false; }
        else if (arg == "--port" && i + 1 < argc) settings.port = std::stoi(argv[++i]);
        else if (arg == "--gui") firstRun = true;
        else if (arg == "--bench") benchMode = true;
        else if (arg == "--auto-connect") autoConnect = true;
    }

    while (true) {
        NetMuxFramework framework;

        if (benchMode) framework.EnableBenchmarking(true);

        if (firstRun && !autoConnect) {
            if (!ConfigGUI::ShowDialog(settings, &framework.GetSyncModule())) return 0;
            configManager.Save(settings);
        }

        if (!framework.Initialize(settings)) {
            std::cerr << "Framework init failed." << std::endl;
            if (autoConnect || argc > 1) {
                std::cerr << "Exiting due to initialization failure." << std::endl;
                return 1;
            }
            firstRun = true; autoConnect = false;
            continue;
        }

        framework.GetInputEngine().Initialize(settings.inputConfig);

        framework.SetCursorColor(colorR, colorG, colorB);

        std::thread frameworkThread([&]() {
            framework.Run();
        });

        ConfigGUI::Initialize(settings, &framework.GetSyncModule());

        bool restartRequested = false;
        while (true) {
            if (ConfigGUI::IsRunning()) {
                ConfigGUI::Tick();
            } else if (!autoConnect) {
                // Exit if GUI is closed and we aren't in auto-connect mode
                break;
            }

            framework.GetInputEngine().Update();
            
            if (!framework.IsRunning()) break;

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        framework.Shutdown();
        if (frameworkThread.joinable()) frameworkThread.join();

        // If the GUI was closed by user, exit app.
        // If we want a "Restart" button, we'd set restartRequested = true.
        break;
    }

    return 0;
}
