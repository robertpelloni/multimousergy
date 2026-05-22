#include "OverlayEngine.hpp"
#include <iostream>

OverlayEngine::OverlayEngine() : m_active(false) {}

OverlayEngine::~OverlayEngine() {
    Shutdown();
}

bool OverlayEngine::Initialize() {
    std::cout << "[Overlay] Initializing D3D11/Layered Window..." << std::endl;
    // TODO: Setup D3D11 device and swap chain or transparent layered window
    m_active = true;
    return true;
}

void OverlayEngine::Render(int cursorX, int cursorY) {
    if (!m_active) return;
    // TODO: Draw cursor sprite at (cursorX, cursorY)
}

void OverlayEngine::Shutdown() {
    if (m_active) {
        std::cout << "[Overlay] Shutting down overlay..." << std::endl;
        m_active = false;
    }
}
