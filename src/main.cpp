#include <iostream>
#include <algorithm>
#include "DriverInterface.hpp"
#include "InputEngine.hpp"
#include "NetworkManager.hpp"
#include "OverlayEngine.hpp"
#include "ConfigManager.hpp"
#include "ConfigGUI.hpp"

#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    std::cout << "NetMux starting..." << std::endl;

    ConfigManager configManager("netmux.cfg");
    AppSettings settings = { false, "127.0.0.1", 5555, {0, 0, false} };

    if (!configManager.Load(settings)) {
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
            if (!ConfigGUI::ShowDialog(settings)) return 0;
            configManager.Save(settings);
        }
    }

    NetworkManager network;
    InputEngine input;
    DriverInterface driver;
    OverlayEngine overlay;

    if (settings.isServer) {
        if (!network.StartServer(settings.port)) {
            std::cerr << "Failed to start server on port " << settings.port << std::endl;
            return 1;
        }
    } else {
        if (!network.Connect(settings.remoteIp, settings.port)) {
            std::cerr << "Failed to connect to " << settings.remoteIp << ":" << settings.port << std::endl;
            return 1;
        }
    }

    if (!driver.Initialize() || !input.Initialize(settings.inputConfig) || !overlay.Initialize()) {
        std::cerr << "Failed to initialize core components." << std::endl;
        return 1;
    }

    // Basic loop for demonstration
    bool running = true;
    int remoteX = 0, remoteY = 0;

    while (running) {
        input.Update();

        Packet outPkt;
        while (input.GetPendingPacket(outPkt)) {
            if (input.IsCaptured()) {
                network.SendPacket(outPkt);
            }
        }

        Packet inPkt;
        while (network.ReceivePacket(inPkt)) {
            if (inPkt.type == PacketType::Movement) {
                remoteX += inPkt.x;
                remoteY += inPkt.y;

                // Clamp to screen bounds
#ifdef _WIN32
                remoteX = std::max(0, std::min(remoteX, (int)GetSystemMetrics(SM_CXSCREEN)));
                remoteY = std::max(0, std::min(remoteY, (int)GetSystemMetrics(SM_CYSCREEN)));
#endif

                overlay.Render(remoteX, remoteY);
                driver.SendMouseMovement(inPkt.x, inPkt.y);
            } else if (inPkt.type == PacketType::Click) {
                input.PerformWarpClickRestore(remoteX, remoteY, inPkt.button, inPkt.down);
                driver.SendMouseButton(inPkt.button, inPkt.down);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}
