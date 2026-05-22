#include "DriverInterface.hpp"
#include <iostream>

DriverInterface::DriverInterface() : m_initialized(false) {}

DriverInterface::~DriverInterface() {
    Shutdown();
}

bool DriverInterface::Initialize() {
    std::cout << "[Driver] Initializing virtual HID device..." << std::endl;
    // TODO: Integrate with ViGEmBus or Interception
    m_initialized = true;
    return true;
}

void DriverInterface::Shutdown() {
    if (m_initialized) {
        std::cout << "[Driver] Shutting down virtual HID device..." << std::endl;
        m_initialized = false;
    }
}

bool DriverInterface::SendMouseMovement(long dx, long dy) {
    if (!m_initialized) return false;
    // std::cout << "[Driver] Moving mouse: " << dx << ", " << dy << std::endl;
    return true;
}

bool DriverInterface::SendMouseButton(int button, bool down) {
    if (!m_initialized) return false;
    std::cout << "[Driver] Button " << button << (down ? " down" : " up") << std::endl;
    return true;
}
