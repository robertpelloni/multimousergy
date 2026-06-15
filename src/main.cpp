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
        else if (arg == "--boundary-x" && i + 1 < argc) settings.inputConfig.boundaryX = std::stoi(argv[++i]);
        else if (arg == "--boundary-y" && i + 1 < argc) settings.inputConfig.boundaryY = std::stoi(argv[++i]);
        else if (arg == "--left") settings.inputConfig.isLeft = true;
        else if (arg == "--right") settings.inputConfig.isLeft = false;
        else if (arg == "--gui") firstRun = true;
        else if (arg == "--bench") benchMode = true;
        else if (arg == "--auto-connect") autoConnect = true;
        else if ((arg == "--key" || arg == "-k") && i + 1 < argc) settings.securityKey = argv[++i];
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

#ifdef __linux__
        framework.GetInputEngine().Initialize(settings.inputConfig, framework.m_xDisplay);
#else
        framework.GetInputEngine().Initialize(settings.inputConfig);
#endif

        framework.SetCursorColor(colorR, colorG, colorB);

        std::thread frameworkThread([&]() {
            framework.Run();
        });

        ConfigGUI::Initialize(settings, &framework.GetSyncModule(), &framework.GetNetworkManager(), &framework.GetFileTransferEngine());

        bool restartRequested = false;
        while (true) {
            if (ConfigGUI::IsRunning()) {
                ConfigGUI::Tick();
                // Check if user clicked "Save & Start" which sends WM_USER+1
                // and triggers re-init in main loop
            } else if (!autoConnect) {
                break;
            }

            framework.GetInputEngine().Update();
            
            if (!framework.IsRunning()) break;

            // Simple way to handle restart without complex message passing:
            // The dialog ShowDialog blocks and returns when "Save & Start" is clicked.
            // But we are using Initialize/Tick.
            // Let's modify ConfigGUI to have a restart flag.
            if (ConfigGUI::RestartRequested()) {
                restartRequested = true;
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        framework.Shutdown();
        if (frameworkThread.joinable()) frameworkThread.join();

        if (restartRequested) {
            firstRun = true;
            continue;
        }

        // If the GUI was closed by user, exit app.
        // If we want a "Restart" button, we'd set restartRequested = true.
        break;
    }

    return 0;
}
