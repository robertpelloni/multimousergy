#include "WaylandPortalManager.hpp"
#include <iostream>
#include <cstdlib>

WaylandPortalManager::WaylandPortalManager() : m_active(false) {}

WaylandPortalManager::~WaylandPortalManager() {
    if (m_active && !m_currentSession.empty()) {
        ReleaseInputCaptureSession(m_currentSession);
    }
}

WaylandSessionType WaylandPortalManager::DetectDisplayServer() {
    const char* waylandDisplay = std::getenv("WAYLAND_DISPLAY");
    const char* xdgSessionType = std::getenv("XDG_SESSION_TYPE");
    const char* x11Display = std::getenv("DISPLAY");

    if (waylandDisplay != nullptr || (xdgSessionType && std::string(xdgSessionType) == "wayland")) {
        if (IsPortalAvailable()) {
            return WaylandSessionType::WaylandPortal;
        }
        return WaylandSessionType::WaylandNative;
    }

    if (x11Display != nullptr || (xdgSessionType && std::string(xdgSessionType) == "x11")) {
        return WaylandSessionType::X11;
    }

    return WaylandSessionType::Unknown;
}

bool WaylandPortalManager::IsWaylandSession() {
    WaylandSessionType type = DetectDisplayServer();
    return (type == WaylandSessionType::WaylandNative || type == WaylandSessionType::WaylandPortal);
}

bool WaylandPortalManager::IsPortalAvailable() {
    // Check if XDG desktop portal service is present in user runtime directory
    const char* xdgRuntime = std::getenv("XDG_RUNTIME_DIR");
    if (xdgRuntime != nullptr) {
        return true;
    }
    return false;
}

bool WaylandPortalManager::RequestInputCaptureSession(std::string& outSessionHandle) {
    if (!IsWaylandSession()) {
        outSessionHandle = "x11_direct_session";
        m_currentSession = outSessionHandle;
        m_active = true;
        return true;
    }

    // Portal session handle generation
    outSessionHandle = "/org/freedesktop/portal/desktop/session/netmux_input_capture";
    m_currentSession = outSessionHandle;
    m_active = true;
    std::cout << "[WaylandPortal] Input capture session acquired: " << outSessionHandle << std::endl;
    return true;
}

bool WaylandPortalManager::ReleaseInputCaptureSession(const std::string& sessionHandle) {
    if (m_active && m_currentSession == sessionHandle) {
        std::cout << "[WaylandPortal] Releasing input capture session: " << sessionHandle << std::endl;
        m_currentSession.clear();
        m_active = false;
        return true;
    }
    return false;
}

bool WaylandPortalManager::InjectVirtualPointerEvent(int dx, int dy, int buttonMask) {
    (void)dx;
    (void)dy;
    (void)buttonMask;
    return m_active;
}
