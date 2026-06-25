#include <iostream>
#include <algorithm>
#include "NetMuxFramework.hpp"

#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    #ifdef _WIN32
    SetProcessDPIAware();
    #endif
    std::cout << "NetMux starting..." << std::endl;

    ConfigManager configManager("netmux.cfg");
    AppSettings settings = { false, "127.0.0.1", 5555, {0, 0, false} };

    configManager.Load(settings);

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--server") settings.isServer = true;
        else if (arg == "--client" && i + 1 < argc) { settings.remoteIp = argv[++i]; settings.isServer = false; }
        else if (arg == "--port" && i + 1 < argc) settings.port = std::stoi(argv[++i]);
        else if (arg == "--boundary-x" && i + 1 < argc) settings.inputConfig.boundaryX = std::stoi(argv[++i]);
        else if (arg == "--boundary-y" && i + 1 < argc) settings.inputConfig.boundaryY = std::stoi(argv[++i]);
        else if (arg == "--left") settings.inputConfig.isLeft = true;
        else if (arg == "--right") settings.inputConfig.isLeft = false;
        else if (arg == "--auto-connect") settings.autoConnect = true;
        else if ((arg == "--key" || arg == "-k") && i + 1 < argc) settings.securityKey = argv[++i];
    }

    NetMuxFramework framework;

    if (!framework.Initialize(settings)) {
        std::cerr << "Framework init failed." << std::endl;
        return 1;
    }

#ifdef __linux__
    framework.GetInputEngine().Initialize(settings.inputConfig, framework.m_xDisplay);
#else
    framework.GetInputEngine().Initialize(settings.inputConfig);
#endif

    framework.SetCursorColor(settings.peerColorR, settings.peerColorG, settings.peerColorB);

    std::thread frameworkThread([&]() {
        framework.Run();
    });

    while (framework.IsRunning()) {
        framework.GetInputEngine().Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    framework.Shutdown();
    if (frameworkThread.joinable()) frameworkThread.join();

    return 0;
}
