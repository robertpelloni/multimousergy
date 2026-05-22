#include "NetworkManager.hpp"
#include <iostream>

NetworkManager::NetworkManager() : m_running(false) {}

NetworkManager::~NetworkManager() {
    Shutdown();
}

bool NetworkManager::StartServer(int port) {
    std::cout << "[Network] Starting server on port " << port << "..." << std::endl;
    m_running = true;
    return true;
}

bool NetworkManager::Connect(const std::string& address, int port) {
    std::cout << "[Network] Connecting to " << address << ":" << port << "..." << std::endl;
    m_running = true;
    return true;
}

void NetworkManager::SendPacket(const Packet& packet) {
    if (!m_running) return;
    // TODO: Implement UDP/TCP sending logic
}

bool NetworkManager::ReceivePacket(Packet& packet) {
    if (!m_running) return false;
    // TODO: Implement UDP/TCP receiving logic
    return false;
}

void NetworkManager::Shutdown() {
    if (m_running) {
        std::cout << "[Network] Shutting down connections..." << std::endl;
        m_running = false;
    }
}
