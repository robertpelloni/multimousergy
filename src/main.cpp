#include <iostream>
#include <algorithm>
#include "NetMuxFramework.hpp"

#include <thread>
#include <chrono>
#include <string>

// We need an approach to parse JSON without adding a heavy library if we don't have one.
// Let's check if there's a simple parsing logic we can use, or write a lightweight one.
// Since we expect `{"command":"send_file","path":"...","target":0}`, we can use a naive string search.

void HandleStdin(NetMuxFramework& framework) {
    std::string line;
    while (framework.IsRunning() && std::getline(std::cin, line)) {
        if (line.find("\"command\":\"send_file\"") != std::string::npos) {
            // Extract path
            size_t pathPos = line.find("\"path\":\"");
            if (pathPos != std::string::npos) {
                pathPos += 8;
                size_t pathEnd = line.find("\"", pathPos);
                if (pathEnd != std::string::npos) {
                    std::string path = line.substr(pathPos, pathEnd - pathPos);

                    // Unescape backslashes if any (basic support)
                    std::string cleanPath;
                    for (size_t i = 0; i < path.length(); ++i) {
                        if (path[i] == '\\' && i + 1 < path.length()) {
                            cleanPath += path[i + 1];
                            ++i;
                        } else {
                            cleanPath += path[i];
                        }
                    }

                    // Extract target (basic support)
                    unsigned long long target = 0;
                    size_t targetPos = line.find("\"target\":");
                    if (targetPos != std::string::npos) {
                        targetPos += 9;
                        size_t targetEnd = line.find("}", targetPos);
                        if (targetEnd != std::string::npos) {
                            try {
                                target = std::stoull(line.substr(targetPos, targetEnd - targetPos));
                            } catch (...) {}
                        }
                    }

                    std::cout << "[Main] Received send_file command for path: " << cleanPath << " to target: " << target << std::endl;
                    framework.GetFileTransferEngine().StartTransfer(cleanPath, target);
                }
            }
        }
    }
}

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

    std::thread stdinThread([&]() {
        HandleStdin(framework);
    });

    while (framework.IsRunning()) {
        framework.GetInputEngine().Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // Force close stdin thread by shutting down (though std::getline can block)
    // We exit process gracefully below
    framework.Shutdown();
    if (frameworkThread.joinable()) frameworkThread.join();

    // std::cin might be blocked, so we detach if it's still running
    if (stdinThread.joinable()) stdinThread.detach();

    return 0;
}
