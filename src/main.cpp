#include <iostream>
#include <algorithm>
#include "DriverInterface.hpp"
#include "InputEngine.hpp"
#include "NetworkManager.hpp"
#include "OverlayEngine.hpp"

#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    std::cout << "NetMux starting..." << std::endl;

    bool isServer = false;
    std::string remoteIp = "127.0.0.1";
    int port = 5555;
    Config config = { 0, 0, false };

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--server") {
            isServer = true;
        } else if (arg == "--client" && i + 1 < argc) {
            remoteIp = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--boundary-x" && i + 1 < argc) {
            config.boundaryX = std::stoi(argv[++i]);
        } else if (arg == "--left") {
            config.isLeft = true;
        }
    }

    NetworkManager network;
    InputEngine input;
    DriverInterface driver;
    OverlayEngine overlay;

    if (isServer) {
        if (!network.StartServer(port)) {
            std::cerr << "Failed to start server on port " << port << std::endl;
            return 1;
        }
    } else {
        if (!network.Connect(remoteIp, port)) {
            std::cerr << "Failed to connect to " << remoteIp << ":" << port << std::endl;
            return 1;
        }
    }

    if (!driver.Initialize() || !input.Initialize(config) || !overlay.Initialize()) {
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
            network.SendPacket(outPkt);
        }

        Packet inPkt;
        if (network.ReceivePacket(inPkt)) {
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
