#include <iostream>
#include <algorithm>
#include "NetMuxFramework.hpp"
#include "ConfigGUI.hpp"

#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    std::cout << "NetMux starting..." << std::endl;

    ConfigManager configManager("netmux.cfg");
    AppSettings settings = { false, "127.0.0.1", 5555, {0, 0, false} };

    unsigned char colorR = 255, colorG = 0, colorB = 0;
    bool benchMode = false;
    bool firstRun = !configManager.Load(settings);
    if (firstRun) {
        std::cout << "No configuration file found. Using defaults." << std::endl;
    }

    // Override with command line arguments if any
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--server") {
            settings.isServer = true;
        } else if (arg == "--client" && i + 1 < argc) {
            settings.remoteIp = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            settings.port = std::stoi(argv[++i]);
        } else if (arg == "--boundary-x" && i + 1 < argc) {
            settings.inputConfig.boundaryX = std::stoi(argv[++i]);
        } else if (arg == "--left") {
            settings.inputConfig.isLeft = true;
        } else if (arg == "--gui") {
            firstRun = true;
        } else if (arg == "--color" && i + 3 < argc) {
            colorR = (unsigned char)std::stoi(argv[++i]);
            colorG = (unsigned char)std::stoi(argv[++i]);
            colorB = (unsigned char)std::stoi(argv[++i]);
        } else if (arg == "--bench") {
            benchMode = true;
        }
    }

    NetMuxFramework framework;

    // Optional Peer Discovery Phase
    if (firstRun) {
        std::cout << "Searching for peers (5 seconds)..." << std::endl;
        auto start = std::chrono::steady_clock::now();
        NetworkManager tempNetwork;
        while (std::chrono::steady_clock::now() - start < std::chrono::seconds(5)) {
            DiscoveryPacket dpkt;
            if (tempNetwork.PollDiscovery(dpkt)) {
                std::cout << "Discovered peer: " << dpkt.hostname << " on port " << dpkt.port << std::endl;
                settings.remoteIp = dpkt.hostname;
                settings.port = dpkt.port;
                // For alpha, we take the first discovered peer as primary
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    if (firstRun) {
        if (!ConfigGUI::ShowDialog(settings)) return 0;
        configManager.Save(settings);
    }

    if (!framework.Initialize(settings)) {
        return 1;
    }

    // Set custom color if provided via CLI
    framework.SetCursorColor(colorR, colorG, colorB);

    if (benchMode) {
        std::cout << "[Bench] Benchmarking mode enabled." << std::endl;
        framework.EnableBenchmarking(true);
    }

    framework.Run();

    return 0;
}
