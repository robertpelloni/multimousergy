#include <iostream>
#include "DriverInterface.hpp"
#include "InputEngine.hpp"
#include "NetworkManager.hpp"
#include "OverlayEngine.hpp"

#include <thread>
#include <chrono>

int main() {
    std::cout << "NetMux starting..." << std::endl;

    NetworkManager network;
    InputEngine input;
    DriverInterface driver;
    OverlayEngine overlay;

    if (!driver.Initialize() || !input.Initialize() || !overlay.Initialize()) {
        std::cerr << "Failed to initialize core components." << std::endl;
        return 1;
    }

    // Basic loop for demonstration
    bool running = true;
    while (running) {
        input.Update();

        Packet pkt;
        if (network.ReceivePacket(pkt)) {
            if (pkt.type == PacketType::Movement) {
                overlay.Render(pkt.x, pkt.y);
                driver.SendMouseMovement(pkt.x, pkt.y);
            } else if (pkt.type == PacketType::Click) {
                driver.SendMouseButton(pkt.button, pkt.down);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}
